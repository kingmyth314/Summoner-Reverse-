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
the drawing code; new_help.cpp is left failing under VC6 until that unit is revisited. (Resolved by D6500: a [][2] array of a one-reference struct, same code and data.)

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

**D6500 (game/new_help.cpp; resolves D4902's open point). The help tables are two-dimensional arrays of a
one-reference struct; new_help.cpp compiles under VC6.** VC6 12.00.8168 brace-initialises neither a struct with two
reference members (C2440) nor an aggregate whose members are classes with constructors (C2552 "non-aggregate"), so
the struct `help_line { char *&key; char *&text; }` cannot be written in any brace form, and `help_line(a, b)`
elements load both addresses into registers first (D4902). What the original can have written, and what keeps the
binary's data layout (two dwords per row, key first) and code: `help_string help_mouse_lines[][2] = { { {XSTR(..)},
{XSTR(..)} }, ... }` with `struct help_string { char *&str; }` (the converting constructor `#ifdef _MSC_VER`, as
saveload_month_name, D4902), the rows read as `[i][0].str` (key) and `[i][1].str` (description). The address
arithmetic is the old one. Verified: the five `$E` initialisers are store for store and in order the binary's
init_strptrs_5bc928 / _5bc968 / _5bc980 / _5bc9e8 / _5bca38 (0x40bf40, 0x40c000, 0x40c050, 0x40c170, 0x40c250: 16,
6, 26, 20 and 8 `mov dword [table+4k], offset localize_strings+4id`, then `ret`; checked from the listing, the tool
does not pair `$E` functions), and the four help_draw_page_* functions match byte for byte. mingw (no _MSC_VER)
brace-initialises the reference aggregate unchanged.

**D6501 (game/new_help.cpp). help_frame: the ESC screen is an if / else of two calls.** `interface_set_screen((p & 2)
? 14 : 0, 0)` gives 8168's `and / neg / sbb / and` select; the binary's `push 0; test al, 2; je; push 0xe; jmp; push
0` is what 8168 makes of `if (p & 2) interface_set_screen(14, 0); else interface_set_screen(0, 0);` (the shared
`push 0` and call cross-jumped). Same shape: buysell_button_action's `buysell_set_category(mode == 0 ? 1 : 0)`
(D6005's gr_glide_flip form). Both match.

**D6502 (the nine units of this pass). Source shapes that make cl 12.00.8168 emit build 8966's code** (each
verified by a byte match; the pass notes have the details):
- Polarity of an early exit: where the binary's fall-through after the test is the main path, the source is `if
  (ok) { main path; return; } failure;` (party_order_attack, buysell_buy_item, phoenix_pick_weighted_move's `if
  (pick != -1) return ...; return -1;`), and `if (count <= 0) { simple; } else { computed; }` where the binary jumps
  over the simple part (saveload_update_scrollbar, mp_charselect_update_scrollbar).
- Nested ifs with one shared failure return (D6205): ghost_seat_rider, phoenix_seat_rider (`if (mounted) { if
  (mount) { ...; return true; } } return false;`). cmd_phoenix_unlink does its work inside the search loop and
  returns there (the loop exit is the not-found return).
- Mouse-up / held order of the *_update_buttons template: `if (!mouse_is_down(0)) { release; } else { held; return;
  }` with the element pointer `ui_button_state *b = &buttons[button]` (saveload, buysell, charselect).
- Element pointers (D5904 / D6205): `ui_button_state *b = &buttons[i]` and `ui_button_def *d = &defs[i]` in the
  three *_load_bitmaps loops, the draw loops (saveload_draw, buysell_draw, mp_charselect_draw) and the category
  tooltip; the button-clearing loop of buysell_frame and buysell_draw's loop are index loops (`jl` on the pointer),
  not pointer loops (`jb`). The opposite twice: mp_charselect_draw_char_list reads `mp_char_profiles[index].name`
  directly (no record pointer), container_examine indexes lc_entries[i] (improves, still differs).
- Rectangle arguments through locals in x, y, w, h order (D6205): saveload_hover_thumbnail, mp_charselect_frame's two
  focus tests, saveload_handle_input's list tests (improve).
- Clamps: `pos += page; if (pos < max) max = pos; scroll = max;` gives the binary's single store of a min
  (saveload_page_down, saveload_drag_scrollbar, buysell_page_down, buysell_drag_scrollbar); `x = c ? a : b` stores
  the first value and overwrites it.
- Vector expressions instead of components: `*out = owner->orient.fvec * s + owner->pos` (predict_owner_position),
  `c->pos - boss->pos` (machival_knock_down_party, machival_teleport, luminar_choose_melee_move's `t->pos -
  owner->pos - luminar_bite_tag_pos`), `pt->pos = pos + c->centre_offset` (update_attack_points), `(cur_character->pos
  + owner->pos) * 0.5f` (operand order of the inline add, phoenix_set_dive_point), `vector3 d(x, 0.0f, z)` for the
  offense point cone tests, and `vector3 dir = vector3(...)` (temporary plus copy, D5904) in phoenix_special_strike.
- Natural x, y, z order of a dot product (8168 reverses it, D6205): machival_offense_point_reaches,
  ghost_point_in_attack_cone.
- A call argument that is itself a call, through a local: `move_info *m = owner->find_move(..);
  owner->play_move((int)m, ..)` (luminar grab approach, start release, teleport in; machival special move), the
  variation local before an if / else of two find_move calls (machival_cast_spell), `int r = rand() % 6;` before the
  table read (machival_teleport).
- Store order: `machival_cast_anim = owner->cur_anim; machival_casting_spell = spell;` gives the binary's casting
  store first (8168 reorders the two stores; machival_cast_spell / _cast_spell_at).
- Smaller ones: `h * (x / c)` for the charselect scrollbar thumb (the fild order; same value), phoenix_rider_set_flinch's
  message buffer is 0x80 bytes (as tiger_rider_set_flinch), `return f() ? false : true` for `test al, al / sete al`
  (luminar_hold_attack), fabsf instead of fabs (world_pick_ground_point, character_can_talk_to: float constants).

**D6510 (include/engine/ai/boss_ai.h, game/unnamed_machival_0x4c0c40.cpp). HEADER CHANGE: the Machival's state
0x248b030-0x248b377 is one object, struct machival_boss machival.** (1) One static initialiser, init_machival_timers
(0x4c0c50), constructs the GameTimers of the five threat records, 0x248b310, the attack, teleport and three spell
timers and the party spell timer: VC6 emits one `$E` function per global with a dynamic initialiser (seen again with
the help tables, D6500), so these are one object (the rule of D1900 / D3403). (2) The binary reloads
machival_num_offense_points after storing a tag into machival_offense_points[n] (machival_boss_load,
update_offense_points) and machival_area_hit_count after storing into machival_area_hits[n] (collect_area_hits,
attack_test); 8168 does that for fields of one object and not for separate globals. With the struct alone three functions
match (0x4c1820, 0x4c1bc0, 0x4c1d10); boss_load's last difference is the
find_mesh_tag argument scheduling the phoenix shares. Extent **inferred**: the move pointers on both sides
(0x248b01c-0x248b02c, machival_move_melee 0x248b378) are kept as separate globals as the phoenix's are; the end of the
.bss block is not proven. Field names ours, the old names are macros (`#define machival_current_move
(machival.current_move)`; the externs of machival_combat_enabled and machival_current_move became comments).
LAYOUT_CHECK 0x348. Callers (boss_ai.cpp, level_scripts_common.cpp, level_script_templeint2.cpp,
unnamed_evil_urath_0x4c4170.cpp) compile unchanged.

**D6511 (include/engine/ai/boss_ai.h, game/unnamed_ghost_rider_0x4b9e70.cpp). HEADER CHANGE: the Ghost Rider's
state 0x2480310-0x248090f is one object, struct ghost_boss ghost.** As D6510: init_ghost_timers (0x4ba080) constructs
the five threat timers and the target, think, wander, fear and summon timers in one function; cut as D3403 (stage
first, up to the next console object ccmd_ghost_unlink 0x2480910); its first 0x140 bytes have the phoenix's field
order (stage, target stage, four flags, a dword, the anim remap, the offense points). The reloads of
ghost_num_offense_points, ghost_num_hits and ghost_remap_count follow. Four functions match with it alone
(update_attack_points 0x4bb080, anim_remap_out 0x4bb6c0, attack_test 0x4bbae0, minions_deactivate 0x4bbdd0). The
move pointers (ghost_move_*), ghost_mount and ghost_loaded, outside the extent, stay separate globals. LAYOUT_CHECK
0x600; ghost_stage, ghost_rider_threat_table, ghost_cur_move and the other externs became macros. Callers (boss_ai.cpp,
spells.cpp, living_entity.cpp, the level scripts) compile unchanged.

**D6512 (notes only). The Luminar's state 0x248cdb0-0x248ce2f is one object too, not converted.**
init_luminar_timers (0x4c8d20) constructs all five of the unit's GameTimers, so by D6510's rule
0x248cdb0-0x248ce2f (luminar_move_anim up to ccmd_stage 0x248ce30) is one object. A struct luminar_boss (0x80 bytes,
the field order of the .bss list) was tried: it compiles and changes no function's code (the unit has no
count-and-array pairs), so the declarations were left as separate globals to keep the header change small. Open for
a later pass.

**D6513 (include/summoner/interface/new_charselect.h, game/new_charselect.cpp). HEADER CHANGE:
mp_charselect_get_selected_profile takes bool.** 0x408540 reads the argument (at 0x408560) as `mov al, [esp+8] / test al, al`; an
int parameter gives `mov eax / test eax, eax`. Every caller passes the constant 0 or 1 (new_charselect.cpp,
new_gamecreate.cpp, new_gamelist.cpp, unnamed_lobby_0x43ef30.cpp, unnamed_netsession_0x440b40.cpp), which a bool
parameter pushes the same way. Matches.

Header checks for D6510-D6513: mingw `-fsyntax-only -std=c++98`: 0 `error:` lines in all 259 .cpp files. VC6 before
(snapD tree) / after (match7 tree), every other unit that names a changed identifier: boss_ai 45 / 45 of 58,
level_script_jadetemple 6 / 6, level_script_rand_forestnite1 4 / 4, level_script_templeint2 15 / 15,
level_script_worldmap 13 / 13, level_scripts_common 64 / 64, living_entity 88 / 88, new_gamecreate 11 / 11,
new_gamelist 19 / 19, spells 18 / 18, unnamed_evil_urath 11 / 11, unnamed_lobby 22 / 22, unnamed_murod_oni 7 / 7,
unnamed_netsession 15 / 15, unnamed_pyrul 14 / 14, unnamed_render_scene 48 / 48, unnamed_swi 12 / 12; the same
addresses (none lost, none won).

**D6514 (notes only). Differences of this pass that no source form changed** (8168 against 8966, as D5905 / D6107 /
D6206):
- Shared return blocks: 8966 jumps to one `xor eax, eax / ret` (or `return 0`) block at the end from several early
  returns (saveload_handle_input's selected-row test, ghost_choose_attack's `handle == -1`, character_can_talk_to);
  8168 returns in place. The same for tails cross-jumped between two switches (machival_ai_frame's client switch
  calls the main switch's special-move block).
- Jump tables with an explicit `case -1: break;` (ghost_ai_frame, phoenix_ai_frame, machival_ai_frame): 8966 gives
  -1 its own byte-table slot (`inc eax; cmp eax, N`), 8168 drops the empty case (`add eax, -15`). The switch on the
  choose result itself is written as a switch now (ghost_ai_frame: closer).
- Loop layout: a do-while whose success exits through a return or goto is laid out `je exit; jmp top` by 8168 and
  `jne top` by 8966, with the success block moved to the end (ghost_wander_start, luminar_teleport_out).
- The `max(x, 0)` AND into the value register (D5905) in saveload_page_up, buysell_page_up,
  buysell_count_party_items, mp_charselect_wrap_text_input and mp_char_create: each otherwise matches.
- The loop bound load scheduled at the loop end (ghost_update_attack_points, sim 0.96); the find_mesh_tag argument
  pushed after its object pointer is loaded (machival / phoenix boss_load, 0.94 / 0.99); stores reordered around a
  call (saveload_save_to_slot's `sete` result).
- A float camera field passed through fld / fstp (saveload_capture_thumbnail; the interface unit's call 0x41176d has the
  same shape): probably an inline accessor in the original camera class; not changed (header).
- A bool local in the top byte of its slot (buysell_goto_inventory, D5905).
- Register choice: the charselect list index in edx / ecx (mp_charselect_page_down, _drag_scrollbar), the nav pointer
  compare of object_open_pickup, count_targets_in_reach's two vectors in swapped slots.

**D6515 (notes only). Markers that say exact but differ under 8168**, left as they were (D6207):
mp_charselect_update_create_button (0x406bb0), buysell_page_up (0x405790), buysell_goto_inventory (0x405bc0),
phoenix_anim_remap_out (0x499b80).

**D6800 (vsdk/gr.cpp, targautils.cpp, file_packfile.cpp, character.cpp, unnamed_clip_0x5590b0.cpp,
unnamed_net_0x517590.cpp; match10 pass). Source shapes that make cl 12.00.8168 emit build 8966's code** (each verified
by a byte match; details in the pass notes):
- Three unfolded constant factors (D6600): frame * (1/30) * 30 * 160 with a float local per product
  (add_skeleton_trigger, set_skeleton_trigger_time); the CA_FRAMES_TO_TICKS macro (D4903) folded two of them and is
  gone.
- Arguments reused as the binary does: `gamma = 1.0f / gamma;` puts the exponent in the argument slot (gr_set_gamma),
  and `powf(...) * 255.0f` gives the binary's float multiplier (`fmul dword`; `pow(...) * 255.0f` multiplies by a
  double); `filename[512]` in pcx_write (the frame is 0x394, the upper 256 bytes are never touched).
- `if (ms <= 0)` for `test eax, eax / jg` (gr_fade_out / gr_fade_in; `ms < 1` compares with 1).
- Declaration order = store order (D6600): `float angle = 0.0f;` before `int segments` (gr_circle), `int total = 0;`
  before `ubyte count = 1;` (pcx_encode_line).
- Loads before stores through locals (D6600): `Mempool *p = pool;` before the vim store (reload_mesh), both pixel
  pointers read before their stores (copy_pixels_advance), the source address copied through a `net_addr` local
  (nw_buffer_push).
- A local walking pointer (`const char *p = name;`): packfile_hash's register choice.
- Two pointer clears as a counted loop give their own zero register (character_clear's LOD slots; as clear_hotkeys).
- `(c & 0x1f) * 8` instead of `<< 3` keeps the binary's byte masks (tga_convert_pixels: 8168 drops a mask before a
  byte shift, keeps it before a multiply).
- The found work inside a search loop, with the single fail return after it: `for (...) if (found) { ...; return; }`
  gives 8966's one shared `return 0` block that D6514 listed as not reproducible (nw_buffer_push exact; nw_buffer_pop,
  nw_accept_reliable and packfile_add, written `if (a && b && c) { ... } return 0;`, now differ only later).
- `while (1) { if (!f(i)) break; ... }` for a top-tested call loop (nw_flush_receive's inner loop, D5904).
- A dead store the binary keeps: `volatile int error` (nw_init_socket keeps the setsockopt error in a frame slot; 8168
  drops a plain dead local and the frame shrinks by 4). A reproduction device like D6101's renames.

**D6801 (include/vsdk/gr/opengl/gr_opengl.h, vsdk/gr_opengl.cpp). HEADER CHANGE: gr_opengl_set_antialias takes a
bool.** gr_set_antialias pushes its bool argument's dword unmasked for the OpenGL call as for the Direct3D and Glide ones
(0x5080e1-0x5080ea, no `and eax, 0xff`); the function is an empty `ret` (0x550630), so its own code says nothing. The
definition in gr_opengl.cpp follows. gr_set_antialias matches; gr_opengl.cpp 31 / 55 before and after.

**D6802 (include/vsdk/gr/unnamed_clip_0x5590b0.h, vsdk/unnamed_clip_0x5590b0.cpp). HEADER CHANGE:
bbox_straddles_plane is (min, max, plane).** The plane is read from the third argument (`mov esi, [esp + 0x2c]` at
0x559b8f, used as `this` of plane::distance_to_point and as the normal for bbox_corner_indices_from_normal), the box
from the first two (pushed as bbox_get_corner's min / max). No caller in the binary or the tree. Matches.

Header checks for D6801 / D6802: mingw `-fsyntax-only -std=c++98 -w -Iinclude`: 0 `error:` lines in all 259 .cpp. VC6
before (snapE tree) / after (match10 tree) of the other units that include the changed headers: levelrender 5 / 5,
gr_opengl 31 / 31, unnamed_ca_mesh 8 / 8, unnamed_gr_line 2 / 2, unnamed_gr_poly 6 / 6, unnamed_vfx_mesh 3 / 3 (the
same addresses).

**D6803 (notes only; match10 pass). Differences of this pass that no source form changed** (8168 against 8966, as
D5905 / D6005 / D6306 / D6514 / D6603):
- Stack pops not merged across consecutive cdecl calls (character report_memory, character_create_base's and
  add_skeleton's error loops, packfile_init, packfile_alloc_directory): 8168 merges the cleanup of several calls into
  one `add esp`; tried statement and local forms, and every flag set of D6602.
- gr_vertex's constructor (D1605, still open): gr_rect and gr_bitmap_float run `vector constructor iterator` over the
  vertices in the binary.
- Copy elision of vector3 results (D6306): bbox_from_segment and most of fx_recb0 (spawn excepted; the frames are
  12 bytes per temporary larger).
- strcat of a short literal as dword / byte moves (load_shadow's "_shadow.vim", tga_read_header's ".tga", D6005).
- Cross-jumping: 8168 merges a case's `next_start_code(); goto top` with the function's first call
  (m2v_skip_extension_and_user_data); nw_init_networking's shared "none" return is placed at the end by 8168, inline
  by 8966.
- FPU values kept on the stack across compares (gr_classify_sphere_view's nz, fx_recb0 get_fade / get_color), the
  `or eax, eax` of nw_can_write (D6005), the zero pushed from EBP in pcx_read_bitmap, the dword copy of last_t in
  spawn, the strength-reduced arrays of ca_load_preload_index and gr_tmapper_multipass.
- Store scheduling and register choice (packfile_check_header, read_directory, assign_offsets, fseek, fread,
  find_in_packfile, gr_init's gr_screen stores, gr_line, bbox_union / from_points / silhouette_corners, gr_clip_line,
  gr_clip_plane, tga_rle_encode_row, m2v_read_slice_quantizer_scale, nw_buffer_pop's address copy, nw_NewCloseSocket).
Partial progress kept: ca_preload_find (count in a local, walking entry pointer; differs at +0x4a), tga_rle_decode
(int header byte: frame and slots now as the binary, `and ebx` against `and bl`), packfile_read_directory (rotated loop),
packfile_add, nw_buffer_pop, nw_accept_reliable, gr_clip_line, gr_classify_sphere_view.

**D6804 (notes only). Markers that say exact but differ under 8168**, left as they were (D6207):
m2v_read_slice_quantizer_scale (0x554af0), packfile_assign_offsets (0x5340d0), packfile_fseek (0x534830),
packfile_fread (0x5348a0), packfile_find_in_packfile (0x5349a0), gr_clip_line (0x559350), gr_clip_plane (0x5593a0),
bbox_from_points (0x5595e0), bbox_from_segment (0x559660), bbox_union (0x559790), bbox_silhouette_corners (0x559890).

**D6700 (include/engine/rendering/levelrender.h, game/unnamed_render_scene_0x481680.cpp, game/gameplay.cpp). HEADER
CHANGE: hover_tint_active returns bool (amends D4300).** Its only caller, gameplay_update_cursor, tests the result
as a byte (`call 0x482090 / test al, al` at 0x439415); an int result is tested with `test eax, eax`. The callee's
`xor eax, eax / mov eax, 1` is what a bool `return a || b;` gives (D6203). The definition in
unnamed_render_scene_0x481680.cpp changes its return type only and still matches (48 / 77 before and after, the same
addresses); gameplay_update_cursor now matches. mingw: 259 files, 0 errors.

**D6701 (include/engine/ai/boss_ai.h, game/unnamed_pyrul_0x4b87c0.cpp). HEADER CHANGE: the Pyrul's state
0x247fd28-0x24802cf is one object, struct pyrul_boss pyrul (as D6510 / D6511).** init_pyrul_timers (0x4b87d0)
constructs the five threat timers and pyrul_move_timer, pyrul_target_timer, pyrul_timer_a and pyrul_timer_b in one
function, the one-object rule of D6510. The binary reloads pyrul_num_offense_points after storing into
pyrul_offense_points[n] (pyrul_boss_load) and pyrul_num_hits after storing into pyrul_hits[n] (both attack tests);
8168 does that for fields of one object only. Extent as D6511: stage first (0x247fd28; the four bytes 0x247fd24
after ccmd_pyrul_combat are not accessed and stay outside), hits[5] last; the move pointers on both sides
(0x247fcfc-0x247fd14, 0x24802d0-0x24802dc) stay separate globals. Field names ours, LAYOUT_CHECK 0x5a8, the old names
are macros (the externs of pyrul_missile_bone, pyrul_combat_enabled, pyrul_threat_table and pyrul_cur_move higher up
in the header became comments). pyrul_boss_load, pyrul_attack_test_class0 and pyrul_attack_test_class2 now match.
Callers rechecked with VC6 before / after (boss_ai 45 / 58, level_scripts_common 64 / 78, level_script_khosanilab2
5 / 5, living_entity 88 / 132, unnamed_ghost_rider 31 / 38, unnamed_titus 19 / 27: identical address lists) and a
full VC6 sweep of all 259 units (4107 -> 4161 matches, none lost); mingw: 259 files, 0 errors.

**D6702 (the ten units of this pass; match9). Source shapes that make cl 12.00.8168 emit build 8966's code** (each
verified by a byte match; details in the match9 notes):
- Element pointers for both arrays of a button loop (D6403 / D6502): pickup / options / levelselect load_bitmaps (the
  def pointer declared right after the state pointer), options_handle_click (`ui_button_def *d` declared first: the
  loop compares the def pointer), pickup_handle_click (`d->flags`, which also gives the `mov bl, 4` constant), the
  draw loops (options_draw declares the pointer after the slider switch); index loops (`jl`) instead of pointer
  loops for pickup_frame and pickup_draw.
- The *_update_buttons template `if (!mouse_is_down(0)) { release; } else { held; return; }` with the element
  pointer (pickup, levelselect); options_update_buttons needs only the pointer.
- Clamps `last = n - page; if (pos < last) last = pos; pos = last;` (pickup_page_down, pickup_drag_scrollbar).
- Rectangle arguments through locals (pickup_list_handle_click, the row test of pickup_handle_click with the first
  row in a local `first`, so that `(i - first) * fh` is strength-reduced as in the binary).
- A call argument that is a call goes into a local (D6005): `move_info *m = owner->find_move(..); play_move((int)m,
  ..)` (tentacle grab / release / death), `int music = music_find_by_name(..)` (khosani salamanka_fight),
  `char *fmt = XSTR(..)` before the sprintf with a call argument (party_member_join), `float fog_near =
  fog_get_near_current()` (game_render_frame, closer). The opposite for strchr: `const char *text = topic->text;`
  (topic_talker).
- Distances: `vector3 v = a - b; float d = v.len();` for the five worldmap nearest_*_nav searches (the binary sums
  z, x, y; no order of three component products gives it); `vector3 d(x, 0.0f, z)` constructed in place and the
  natural x, y, z dot product (pyrul_point_in_attack_cone, D6502); `t->throw_velocity = d * (2.0f / 3.0f)` through
  the inline operator* (tentacle ai_release); `pt->pos = pos + c->centre_offset` (pyrul_update_attack_points).
- Bool shapes: `x ? true : false` for a bool call stored to a bool (`*is_town = tf.Accept(..) ? true : false`,
  read_town_characters) and for `return t->num_hits > 0 ? true : false` (collect_area_hits: `setg al` without the
  `xor eax, eax`); `if (f->error() == 0) return true; return false;` (s3d_read_header, D6005); an early-exit chain
  into one shared `return true` written as one `||` condition (ls_worldmap1_roll_encounter); `if (valid) { ...
  return ..; } return false;` (khosani cell_open).
- Store / statement order: `pickup_container = c;` before the memset (pickup_init); `state = 1;` before `busy = 1;`
  (open_dialogue); the add_point index written as `n = len; ...[n] ..;
  n++; len = n; count = n;` (khosani add_point); `terrain = 0;` after the first distance call (worldmap
  pick_encounter_early); g1 before g0 (khosani idle_path); the float copies of turn_dirs through `y` then `x` locals;
  `slot++` as its own statement after place_at_party_start (khosani puzzle_use).
- Globals the binary keeps in a register while storing: the level list count written as
  `mp_levelselect_levels[mp_levelselect_list_count[1]++] = i` (8168 promotes it, with the first store of 0 kept);
  `int n = num_lobjs;` before the countdown loop and `lobjs[k]` (index local, the global pointer reloaded) in
  level_load_layers.
- Operand order: the options slider fill width `w = btn.w / 2 - pos.x + btn.x` computed first with the x / y / h
  locals declared after it; levelselect thumb `h * (x / c)` (the fild order; same value, D6502);
  `float t = (gamma - 0.7f) / 1.5f;` before the product (options_apply_gamma).
- Frames: pickup_take_selected's message buffer is 128 bytes (D6704); level_load_layers' name buffer 32 bytes;
  block-scoped buffers that share the frame with later temporaries (s3d_read_navpoint's name, s3d_read_geometry's
  name and corner array; both closer, see D6703).
- Switch: case bodies written out separately where the binary's dec chain has two cases jumping to one target
  (options_set_texture_quality: `case 0:` and `case 1:` each with the body; a shared `case 0: case 1:` gives 8168's
  range compare).

**D6703 (notes only; match9). Differences of this pass that no source form changed** (8168 against 8966, as D5905 /
D6206 / D6306 / D6405 / D6514 / D6603):
- Shared return / tail blocks and cross-jumped tails (tentacle ai_frame and cmd_* tails, pyrul_ai_frame,
  state_game_enter's two failure tails, conversation end / line_talker, worldmap stage9's day / night blocks,
  pyrul_cast_special: 8168 merges the wall-of-fire `return true` into the meteor case's tail, 8966 keeps it first).
- Jump tables with an explicit `case -1` slot (pyrul_ai_frame, pyrul_cast_special).
- The `max(x, 0)` AND into the other register (pickup_page_up, mp_levelselect_add_chat_line's scroll clamp).
- Stack pops merged across a varargs call (level_load_report, level_unload, help_draw_page).
- Dead argument / local slots reused (khosani trace_beam and the three corner_* functions, s3d_read_geometry's loop
  index, level_free_s3d's palette pointer, s3d_read_color_record, worldmap stage15's rolls).
- Float members copied through the FPU into a stack temporary (ls_worldmap1_frame's set_float arguments,
  emit_segment's grid points, options_drag_sliders' slider width).
- Bitfield tests through `shr / test cl, 1` (conversation end / select_topic) and the byte narrowing of
  `!(game_flags & 0x20)` (`not al / shr al, 5`, game_render_frame).
- Copy from a constructed temporary through EAX (s3d_read_navpoint's quaternion operator=); the inline
  vector3(float, float, float) called out of line in camera_capture.
- A pointer-compare range test of a switch with `ja` (options_load_bitmaps; 8168 `jg`).
- options_init: 8168 forwards Summoner_options.sfx_volume past the struct copy (also with memcpy).
- Register choice / scheduling: pickup_draw_item_list (ebx / ebp), party_level_reset (8966 pushes the callee-saved
  registers first and keeps CF_SUMMON in EBX), mp_levelselect_draw_levels (pushes after the clamp), read_door's
  `mov ecx, edi`, cmd_tentacles' constant 8 in EBP, mp_levelselect_drag_scrollbar (the operand order of the
  thumb offset is canonicalised by 8168 whatever the source order), state_game_frame's `+=` operand order.

**D6704 (game/new_pickup.cpp). pickup_take_selected's message buffer is 128 bytes; pickup_init stores the container
first.** The frame is 0xc0 with items at +0x10 (16 pointers) and msg at +0x50 (both read from the sprintf / take
loops): msg reaches the end of the frame, so it is char[128], not char[112] with 16 unused bytes. pickup_init's
binary stores pickup_container between the memset's first and second dwords, which is 8168's schedule of the
container store written before the memset. Both now match.

**D6705 (notes only). Markers that say exact but differ under 8168** in this pass's units, left as they were (D6207):
pickup_page_up (0x420d20), level_load_report (0x437d70), level_unload (0x437e00), help_draw_page (0x43a0a0),
conversation_controller::line_talker (0x4d8940), conversation_controller::end (0x4dac30), ls_worldmap1_frame
(0x4fbd80). Functions raised to exact by a VC6 byte match carry "exact (VC6 byte match, match9; was likely ...)".

**D6900 (match11 pass, 28 units; measured with VC6 SP5, c2 12.00.8966). Source shapes that make the original's
compiler emit Sum.exe's code** (each verified by a byte match; details in match11 NOTES.md):
- One-bit bitfield reads cast to bool: `if ((bool)t->listed && !(bool)t->hidden)` gives the binary's `mov ecx, f /
  mov edx, ecx / shr edx, 1 / test dl, 1` (conversation first_topic, next_topic, set_topic, set_speaker,
  set_topic_available, build_topic_list). The plain bitfield test gives `test byte ptr [..], mask`. This overturns
  D6703's "bitfield tests through shr / test cl, 1" entry.
- A bool from a bit: `bool r = x & 1; return r;` gives `and al, 1` (weapon_is_ranged, ai_member_skips_group_orders;
  overturns D6306 (7)); `return p ? true : false;` gives `setne al` without the `xor` (item_has_magic_info).
- One call with a selected constant argument is an if / else of two calls: `if (frand_range(..) < 0) x =
  item_type_find("leather bag"); else x = item_type_find("backpack");` gives `push A / jmp / push B / call`
  (container_obj_create; D6405's entry does not hold for 8966), likewise `if (visible) gr_string(.., buf, ..); else
  gr_string(.., input, ..);` (pxochat_draw_chat_input).
- A pointer from a call into a local before a member copy: `obj *o = obj_from_handle(h); o->pos = l->pos;` (the
  source is read after the call: lsf_ball1_used / lsf_ball2_used, ls_catacombs_sync_object_positions);
  `int music = music_find_by_name("Boss"); music_play(music, true);` (lsf_carados_cutscene, D6702).
- Element pointers for both button arrays (D6403 / D6702): pxochat_load_bitmaps, pxochat_draw (both loops); index
  loops (`jl`) for containers_reset / containers_free_all / ai_groups_frame (`containers[i].deinit()` is the
  binary's direct call: through a pointer VC6 calls the vtable slot); `vector3 *pt = &...offset_points[i].pos`
  (giant_salamanka_target_in_reach).
- Branch layout: pxochat_update_buttons in the release-first template with `int off = 0; if (pressed == 25) off =
  -1;`; mp_trade_frame with the `state != 1` branch first; level_lights_setup with `texture != -1` first;
  ai_groups_frame with the random-timeout branch first; tutorial_frame's Space test through `bool next = key ==
  SPACE; if (!next) next = mouse_was_pressed(0) && tutorial_ok_hit_test(); if (next) ...`.
- Smaller: `(int)strlen(motd) + len <= 1023` for the binary's `cmp ecx, 0x3ff / jg` (pxochat_append_motd); x / y
  locals computed before gr_set_color_ptr (pxochat_draw_caption); `i * font_h` instead of a running y
  (tutorial_draw); the list insert written out with `head->next = e` before `e->prev = head` (build_topic_list; the
  vsdk.h list_insert macro keeps its order, not changed); `level_light_scale * ambient.z` operand order.
- Frames: state_loadgame_enter's file list head is a whole file_find_node (frame 0x3c); pxochat_handle_click's
  mouse deltas are function-scope locals (they do not share the Enter case's line buffer; 0x114 of 0x118 now).

**D6901 (game/unnamed_giant_salamanka_0x4c8030.cpp). LOGIC FIX + GAME BUG: giant_salamanka_set_flinch hangs on a
move without an action.** The skip test at 0x4c829d jumps to the loop test at 0x4c8384, past the next-link load
(0x4c8381 `mov ebx, [ebx + 0x40]`), so a NULL action repeats forever: the source is a while loop whose `continue`
skips `m = m->next`. The decomp had a for loop that stepped past it. Now exact.

**D6902 (game/unnamed_ai_groups_0x492b00.cpp). LOGIC FIX: character_is_helpless(NULL) returns 1.** 0x493047 `je
0x493074` jumps to `mov eax, 1`; the source returned 0 for NULL. `if (!c || sleep || paralyze || ice coffin) return
1;`. Now exact. Callers pass enemies from ai_think_enemies (NULL entries end their loops first).

**D6903 (game/new_tutorial.cpp). LOGIC FIX: tutorial_frame returns true whenever a tutorial is current**, also one
without pages: `mov bl, 1` (0x42ef84) precedes the pages test. The source set it inside the pages test. Now exact.

**D6904 (game/unnamed_cutscene_camera_0x493e50.cpp). Marker fix.** The scalar deleting destructor notes
(0x493f40, 0x493fa0) sat in the same comment block as the destructors' markers, so vc6-compare paired the
destructors with the deleting destructors' addresses and never compared 0x493f60 / 0x493fc0. A blank line separates
them now; both destructors match.

**D6905 (notes only; match11). Still different under 8966** (source forms tried, none matched): store hoisting of
the identical `version_checked = 1; state = 0` pair in the three pxochat popup frames (8966 keeps them in both
branches and shares a zero register with the compare); register choice in the multitrade handlers' first compare
(data and cur_character loads), pxochat_page_down / drag_scrollbar / update_scrollbar / draw_user_list,
bezier4::eval, character_meets_item_requirements; item_create's NULL kept in EAX from the start; the
`max(x, 0)` AND (pxochat_init); FPU sum order (levelrender_sum_point_lights, D6306 (5)); frametime_set_limits (b
copied as an integer); character_instance::render's tint stores after the pushes; state_title_enter's flag
expression; ls_catacombs_use's shared ball1 tail; mp_remove_player's stack pops. Partial progress kept:
state_loadgame_enter (frame, to +0x18c), pxochat_handle_click (element pointers, frame 0x114),
ls_rand_forest01_frame (to +0x1df). Markers that say exact but differ (left as they were): clear_speaker 0x4d7e10,
item_create 0x44b2d0, character_meets_item_requirements 0x44b550, ls_rand_forest01_frame 0x4f5b90, ls_lenele1b_init
0x4fad90, ls_lenele1b_action 0x4fb160, mp_handle_it_acpt 0x4712f0, _cncl 0x471b30, _qury 0x471c50, _stat 0x471d70,
loadgame_load_cb 0x4322e0, frametime_set_limits 0x5016f0, gr_close_fonts 0x5081d0, bezier4::eval 0x558b80,
bezier::eval 0x558d90.

**D7300 (vc6-flags.txt; match15). unnamed_anim_track_0x550660, unnamed_d3d_tcache_0x54bb30 and
unnamed_poly_clip_0x5519c0 are /O2 /Ob0 /GX /MT units.** As D6602 (2) predicted: they call every inline accessor and
vector / cquat operator out of line. Under cl 12.00.8966 (VC6 SP5) with no source change: anim_track 1 -> 10, d3d_tcache
3 -> 7, poly_clip 1 -> 3 of 13 / 14 / 8; no function of the three loses its match. Lines added to vc6-flags.txt.

**D7301 (the 29 units of match15). Source shapes that make cl 12.00.8966 emit Sum.exe's code** (each verified by a
byte match with the SP5 compiler; details in the match15 notes):
- The screen templates of D6403 / D6502 / D6702 hold under 8966 unchanged: element pointers for both button arrays
  (load_bitmaps, handle_click, draw of assess, gamelist, mainmenu), index loops (`jl`) for the flag-clearing loops,
  `case 1: ...; return key;` in the *_frame switch, the `if (!mouse_is_down(0)) { release } else { held; return; }`
  update template, `last = n - page; if (pos < last) last = pos; scroll = last;` clamps (page_down with `int pos =
  scroll; ... pos += page;`, drag_scrollbar), `if (count <= 0) { simple } else { computed }` (update_scrollbar), the
  if / else of two gr_string calls (gamelist_draw_password_input), `switch (key) { case KEY_ESC: ... }` and the
  mouse delta locals at function scope (gamelist_handle_click: frame 0xc).
- A one-bit field tested or used as a bool is cast: `if ((bool)s->proj.inside)`, `t == greeting ||
  (bool)t->initial` give the binary's `shr ecx, n / test cl, 1` (shadow_collect_for_lobj,
  conversation_speaker::refresh_topic_flags). D6703 listed this as not reproducible under 8168.
- A call argument that is a call still goes into a local first under 8966 (D6005): `int music =
  music_find_by_name("Boss"); music_play(music, true);` (ls_rand_forestnite1_frame), `Character *c = owner; int
  variation = spells[spell].variation; c->find_move(7, variation)` (evil_urath_cast_spell: owner loaded first).
- Store order: `cast_anim = owner->cur_anim; casting_spell = spell;` (evil_urath / murod_cast_spell, as machival);
  conversation_speaker::init in the binary's store order (move_in_time, name, topics, tail, greeting, next, flags,
  move_out_time).
- Sums of products: 8966 canonicalises hand-written dot products whatever the order (projector_record::test_bounds:
  all six permutations give the same wrong order); the inline `planes[i].normal.dot(lo->centre) + planes[i].d` gives
  the binary's. A hand-written sqrt of squares in natural x, y, z order (evil_urath_filter_target).
- Bool results: `return Pool ? true : false;` (`test / setne al` without the xor: is_mempool_active); one `||`
  expression for an EAX result (ui_button::is_pressed); `if (tw == w && th == h) return true; return false;` for AL
  (gr_d3d_is_texture_size); one `&&` expression for EAX (vector2::nearly_equal).
- Branch layout: `if (sound != -1) return f(); return -1;` (surface_play_sound: the shared `or eax, -1` at the end),
  `if (p >= num_planes) return 1; return 0;` (polygon_touches_planes), nested ifs written out as three assignments so
  that 8966 cross-jumps the shared call (calc_rot_tangents' previous key).
- Loop-invariant values written inside the loop, so that the compiler computes them after the loop test into the
  dead argument slot (sphere_intersects / sphere_inside_plane_volume's limit).
- Locals: `e = operator new(..); d3d_tcache_entries = e;` and stores through `e` (gr_d3d_tcache_init's countdown loop);
  `s = new ..[n]; n = e->num_sections; e->sections = s;` (create_texture's clear loop); `d3d_tcache_entry *e =
  &entries[frame]` before the section pointer; `anim_rot_key *k = rot_keys;` (get_rot_time_range); `int faces =
  mesh->num_faces;` before the offset store (lobj_face_marks_init); the unlink of a tooltip through `prev` / `next`
  locals (ui_slider_base / ui_button::set_tooltip, as D6005's free-list unlink); `yaw = -yaw;` before the product
  (place_ground_decal: `fchs / fmul` instead of the folded negative constant); the slider size as an if / else of two
  int-field products (`fild [mem]`, ui_slider::update_marker); `num_planes = 0;` written in projector_record::set
  (clear_planes is out of line elsewhere, expanded here).
- Return-value elision works under 8966: get_position / get_rotation / get_scale match unchanged (D6306 (1)).

**D7302 (game/unnamed_evil_urath_0x4c4170.cpp). LOGIC FIX (one ulp): the "silenced or cursed" weight of spell 0x56 is
0x3f5b6db8.** The binary stores 0x3f5b6db8 (6 * (float)(1/7)); the source's `6.0f / 7.0f` is 0x3f5b6db7. Written as
the literal 0.8571429f; the weight table is now if / else stores through an element pointer as in the binary
(evil_urath_ai_select_action still differs in its shared return blocks).

**D7303 (game/gamesound.cpp). sound_play_3d_at passes `false, 0, 0` to snd_play_3d.** The third parameter is a bool;
the source passed 0.0f, 0.0f (converted, the same zero; cl 8966 pushed 0xd8 for the float-to-bool constant). Now exact.

**D7304 (game/unnamed_projectors_0x4c0250.cpp). lobj_project_records' exception frame is a profile_label scope.** As
D6102: the frame with trylevel 0 for the whole body and no unwind object of its own is what an inline profile_label
leaves; `profile_label label("lobj_project_records");` (text ours) gives the binary's code. Now exact.

**D7305 (notes only; match15). Differences that no source form changed under 8966:** max(x, 0)'s AND register
(assess_page_up, gamelist_page_up: every max / ternary / local form tried); shared return / epilogue blocks
(gamelist_handle_click's password branch, murod / evil_urath select_action and ai_frame, mp_server_list_draw); the
explicit `case -1` slot (murod_ai_frame); frame slot sharing between scopes (evil_urath_ai_leap_rise); the owner store
scheduled into the inline strcpy (set_tooltip, D6603); loads scheduled before stores through other pointers
(get_marker_rect, allocate_in_mempool, gr_d3d_tcache_free_entry, gr_d3d_get_section_rect); stack pops not merged
across 16 calls (ls_lenele1aa_init); the `and al, 1` of a bool returned from flags (ui_button::is_latched: probably a
bit field in the original class; header not changed).

**D7400 (compiler; docs/decomp-plan.md "Toolchain").** Sum.exe's C++ objects come from VC6 Service Pack 5:
c2.dll 12.00.8966 (shipped in vs6sp5.exe as `msvcep.dll`, renamed at install for the Professional and
Enterprise editions), with c1xx.dll 12.00.8964, c1.dll 12.00.8867, cl.exe 12.00.8804 and link 6.00.8447. The
`@comp.id` an object carries is c2.dll's build, so this is the build the Rich header names (Utc12_CPP 8966). From
this pass on, every VC6 comparison uses the SP5 compiler. Measured over all 259 files at 2026-10-16 HEAD: 4,179
matches under 8168, 4,231 under 8966 (65 gained, 13 lost).

**D7401 (game/level_scripts_common.cpp, pathfinding.cpp, script_internal.cpp, unnamed_ai_think_0x4b5140.cpp,
unnamed_goals_doors_0x475180.cpp, unnamed_luminar_0x4c8d10.cpp, unnamed_phoenix_0x497ac0.cpp,
unnamed_tiger_0x499e50.cpp). The 13 functions that matched only under 8168**, rewritten for 8966 (12 match; ai
follow_character 0x4b5bd0 still differs). Source shapes 8966 needs where 8168 accepted another:
- a difference used once goes straight into the call: `(a - b).len() < dist`, not `vector3 d = a - b;` (a
  named vector3 makes 8966 spill one component to the frame): 0x4ce3c0, 0x4ce460, 0x4ce4d0, 0x4b68c0;
- the same through the inline `len_sq()` for a hand-written x*x + y*y + z*z: 0x4dda30, 0x4dda90;
- a vector built from a constructor is declared first and assigned (`vector3 d; d = vector3(...);`), which gives
  the z-through-a-temporary copy of the original: 0x4a0330, 0x4a03c0, 0x499890; with an unused result the same
  holds for `vector3 dir; dir = a - b;` (0x49a9b0);
- the order of hand-written squares: 8966 emits them in a different order from 8168, so the source order changes
  (0x4c97b0 x then z; 0x499890 y, x, z);
- a returned vector3 that the original fills through a 12-byte frame local is a local filled field by field and
  returned (0x4756e0).

**D7100 (include/vsdk/gr/gr.h; match13). HEADER CHANGE: gr_vertex derives from vector3 (amends D1605).** The
original's vertex has a vector3 at +0: gr__Vertex__Vertex (0x5081c0) only calls vector3__vector3_default on `this`,
and gr__Vertex__assign (0x540790) assigns +0 through vector3__assign (0x413190, the copy-returning operator=) before
copying the other fields. `struct gr_vertex : vector3 { float sx, sy, sw; ... }` keeps every `v->x / y / z` user
compiling and gives both implicit members. Layout unchanged (0x30). The /Ob0 line unit now calls the constructor on
each local vertex as the binary does: gr_line_3d_world, gr_bitmap_3d, gr_bitmap_3d_oriented, gr_line_quad_3d,
gr_draw_arrow_3d, gr_line_3d_points, gr_draw_arrow_3d_points match (with D7101). All 17 other units that use
gr_vertex rechecked with VC6 8966 (and 8168), old against new header: 0 lost, 1 won
(unnamed_script_editor_0x4de0b0.cpp 0x4e3700); mingw: 259 files, 0 errors. levelrender's static vertex arrays
compile unchanged (the constructor inlines to nothing in /O2 units).

**D7101 (vc6-flags.txt; match13). vsdk/unnamed_gr_line_0x53e520.cpp is a /O2 /Ob0 unit**, as its header already
said (vector3 / matrix operators, gr::Mode::get_alpha_source and gr_clip_free_temp_verts are called out of line in
the binary). With /Ob0 the function sizes agree and gr_draw_axes_3d matches; the line is added to vc6-flags.txt.

**D7102 (match13 units). Source shapes that make cl 12.00.8966 emit the original code** (each verified by a byte
match under VC6 SP5; details in match13 NOTES.md):
- A call argument that is a call goes into a local (D6005 / D6702): `int music = music_find_by_name(..);
  music_play(music, true);` (wolong2 pijian_intro_cs_end, masad barb, iona_ext2 frame; the plain nested form where
  the binary does not merge the pops: iona_ext2's second call), `int r = rand();` before `c->say(localize(lines[r %
  n]), 2.0f)` (lpalaceint sneak_frame, three sites), `char *fmt = XSTR(..);` before a sprintf with a call argument
  (party_add_member, party_remove_member), `int len = pkt.len;` read before the printf that precedes the send
  (gtrack_send_heartbeat).
- Index loops instead of pointer loops where the binary compares the strength-reduced pointer with `jl` (sewer
  tentacle loops, item_info_popup_frame); the element pointer `ui_button_def *d` declared right after the state
  pointer (item_info_popup_load_bitmaps, handle_click with `d->flags`).
- Squared distances: `for (i ..) { vector3 d = c->pos - spike[i]; if (d.len_sq() < r) ... }` (the four wolong
  caverns traps; the binary's z, x, y order and the pointer biased to .z); a horizontal direction built with
  `vector3 d(x, 0.0f, z)` and `d.dot(dir)` (murod_oni_offense_point_reaches).
- Branch layout: `if (SUCCEEDED(hr)) {..} else {..}` (mouse_di_read), `if (event == X) { ...; return true; } return
  false;` (mlayout state6 callbacks), `if (flags & PF_LEADER) {..; return true;} return false;` (drop_leader),
  `if (!Max_skills) {..} else {..}` (Player::create), `if (r != 1) { if (5..6) dropped else left } else kicked`
  (mp_handle_leave_game, closer), the release / held template of D6702 (item_info_popup_update_buttons), one
  combined `||` condition for the shared `return false` (Player::skill_unlocked), `case 2 / 1 / 0 / default` each
  with its own body (credits_frame), `switch (key) { case KEY_ESC: .. }` for the binary's `dec` test (credits_frame).
- `case -1: return;` gives the sub-mode switch its own byte-table slot (murod_oni_ai_frame; `case -1: break;` is
  dropped). The other boss units (ghost rider, machival, murod, phoenix, pyrul, serpent, titus) have `case -1:
  break;` and may match the same way (not tried: not this pass's files).
- Stores and loads: the list macros' order (prev first) for obj_active_list / living_list in party_add_member; the
  head insertion written out with `head.next = elem` before `elem->prev = head` (party_lists_init); a
  `do { .. } while (0)` around the live-list unlink keeps 8966 from moving the flag store into it
  (party_remove_member); `need = level_table[level].unk0c;` before the loop (mp_player_level_available);
  `memset(in, 0, 12)` for the three-dword clear (player_read_mouse_input); `(lparam >> 16) & 0xff` (byte load) for
  the scan code (key_on_keydown / keyup); a cast `(ubyte)data[i]` (BitArray::get: the and narrows to a byte); the
  summon handle kept in the global (evil_joseph_pick_helper_summon, closer); `o = living_from_handle(..)` in one
  obj pointer (party_remove_player); `x ? true : false` for a bool from a pointer compare or a byte
  (murod_oni_move_is_kick, key_numlock_is_on); operand order `credits_scroll_speed * reported_frametime * scale`,
  `skill == progression[i].skill`; `(float)((float)a / (double)b)` for fild / fild / fdivp (credits_draw_lines,
  closer).

**D7103 (vsdk/unnamed_key_0x5093a0.cpp). key_down_state is bool[256].** key_is_down returns the byte as it is
(`mov al, [eax + 0x25da94c] / ret`), which VC6 compiles from a bool array only; the array is defined and used in
this unit only (the header declares key_is_down alone). Now exact.

**D7104 (notes only; match13). Differences that no source form changed under 8966** (D7102's forms tried first):
a global array store followed by a reload of the global index (murod_oni_collect_area_hits, the offense-point
packing loops of murod_oni / evil_joseph boss_load: the binary re-reads the count after the store, 8966 keeps it);
`max(x, 0)` ANDing into the value register (item_info_popup_page_up, four forms); the dead-argument-slot reuse of
gr_line_3d / gr_poly / gr_poly_project_to_screen (D6306 (4) holds for 8966 too); BitArray::all_equal's hoisted
`value != 0`; the strcat of a long literal as a fixed-size rep movsd (DebugError: `strcpy(text + strlen(text), ..)`
is closer but still differs); lfsr get_key's struct copy order; the stores of mp_reset_net_stats through a second
zero register; key_flush's loops (8966 turns them into rep stosd, the binary keeps loops); evil_joseph_spawn_helpers'
temporary (8966 constructs in place for every form tried); the zero register of state6_enter; register choice in
floatmath's table square roots, credits_parse_table and camera_process_input.

**D7200 (match14; 29 units, measured with cl 12.00.8966 = VC6 SP5, the build of Sum.exe's Rich header). Source
shapes that give the original code** (each verified by a byte match under 8966):
- A clamp written as a ternary on the stored value: `p->swi_num_samples = p->swi_num_samples < 16 ? ... : 16`
  (mp_remote_travel_update, the store after SetTimeout's push), `g->alpha = g->alpha <= 0.0f ? 0.0f : g->alpha`
  (lamp_glow_fade_out / _in: the value stays on the FPU stack, stored by fst and fstp). The charge clamps of the
  party inventory take the maximum into a local first: `item_info *e = ...; int m = ...max_charges; e->charges =
  min(e->charges, m);` (item_obj_pickup, party_inventory_add_item / _add_type).
- Search loops with the found work inside the loop (`for (...) if (same) { ...; return; }`), not `break` and a
  second test (item_obj_pickup, party_inventory_add_item / _add_type); add_item's merge as `if (stackable) ...
  else if (charges) ...` with one shared item_free.
- One shared return block written as such: `goto done; ... done: return true;` (character_use_equipped_item),
  a single trailing `return 1` reached from an if / else-if chain (mp_summon_try_claim), `goto none;` to the
  fail path's `return 0` past its close (effect_stream_open_current).
- 8966 merges identical case bodies into one jump-table target; where the binary has separate tests or table
  entries the cases are written with their own bodies: character_unequip_slot (four species cases, each calling
  apply_hero_model(0); the slot cases in the binary's body order TORSO, LEGS, FEET, HANDS, HEAVY), GameInit
  (`case 0:` and `default:` each with the 640 font; cases in the order 2, 1, 0), mp_read_spell_targets (cases in
  the binary's body order 1, 4, 5, 3/6/7, 2/9, 10: closer). A two-value test `x == 29 || x == 31` is a switch
  (the `sub eax, 0x1d / sub eax, 2` chain: mp_read_spell_target_point).
- `x ? true : false` for a bool result or argument (`setcc` without `xor`): GameTimer / RealtimeTimer elapsed and
  valid, LoadSettings' vsync argument of gr_init. GetRemainingTime's last branch is `int d = ticker - value; ...
  return -d;` (the binary's NEG).
- Operand order of a compare: `party_inventory_count > slot + 1` (party_inventory_remove).
- Statement / store order: set_u_axis / set_v_axis (plane index first), NoRlsAllocPool's and StringPool's
  constructors and StringPool::Init (the binary's store order), AddBlock's link through a `prev` local,
  InitBlocks' `can_grow != 0`; a call argument that is a call through a local (`int music =
  music_find_by_name(..); music_play(music, true);`: lsf_tg_intro, lsf_murod_intro, lsf_mach_killed,
  ls_liangshan_frame, the Titus music of ls_ikaemosbottomint2_frame, lsf_ikaemos_found_boss). The other music
  calls of those files stay nested: the split loses ls_templeint2_machival_intro_cs_end and the IkaemosPalace
  call of the ikaemosl2 frame.
- An element pointer in the button draw loop (playmenu_draw, D6403); a do-while for effect_stream_flush; a
  pointer walking the heights of resolution_table (`h < &resolution_table[3][1]`, `jb`) in LoadSettings, with
  `renderer = GR_GLIDE;` written in the 3DFX branch (the binary jumps straight past it).
- `(float)fabs(..)` instead of fabsf in a /Ob0 unit (fabsf is an out-of-line call there): update_pan.
- open_saveload_screen's flag is `bool save[4]` with save[0] written: the byte sits at the bottom of its dword
  slot (a lone bool sits at the top, [esp+3]). A reproduction device like D6101's renames, not a claim about the
  original declaration.
- vc6-flags.txt: vsdk/unnamed_sound_emitter_0x546150.cpp /O2 /Ob0 /GX /MT (D6602's proposal): 15 -> 18 under
  8966, none lost (create_segment, set_pos_vel, update_position).

**D7201 (include/summoner/level_info.h new, summoner/gameplay.h, levelscripts/level_scripts.h, game/gameplay.cpp;
parameter renames in engine/rendering/levelrender.h, levelscripts/conversation.h, game/conversationctl.cpp,
game/unnamed_render_scene_0x481680.cpp). HEADER CHANGE: gameplay.cpp's .bss 0x60a2c8-0x60a40b is one object,
level_info (level_filename, level_script_name, level_prev_filename, level_session_time, level_player_starts[4],
level_entry_point, level_num_layers, level_layer_maps[10], level_layer_automap_index[40]).** Every level change
in the binary stores level_entry_point after the three inline strcpy calls (ls_jadetemple_finish 0x4f0a1f,
ls_rand_hills01_goto_khosanistrng, masad, temple_inside, lsf_tiger2_dead); 8966 hoists that store above the copies
when the names and the entry point are separate globals and keeps it in place only when they are members of one
object (tested on an isolated copy). level_find_player_starts reads level_player_starts[4] as level_entry_point
(sum.symbols 0x4caf40): the same layout. Extent: from level_filename (the function-static GameTimer 0x60a2c0
before it is another object) to the end of level_layer_automap_index; level_is_town (0x60a40c) stays separate.
Struct and field names ours, the old names are macros (as D6701); two parameters named level_filename became
level_name so that the macro does not reach them. Whole tree (259 units, 8966): none lost; won
ls_jadetemple_finish, lsf_tiger2_dead, the courtyard unit's 0x4facc0, masad 0x4fdcd0, rand_hills01 0x4fa6f0,
temple_inside 0x4f6340. mingw: 259 files, 0 errors.

**D7202 (include/vsdk/geom/vfx.h, vsdk/unnamed_attached_lights_0x51fb10.cpp, game/unnamed_fx_0x489c40.cpp). HEADER
CHANGE: attached_light::type is a signed char.** attached_light_free (0x5201f0) loads the free marker with `or cl,
-1`; 8966 emits that for a char -1 and `mov cl, 0xff` for a ubyte 0xff (tested on an isolated copy). The free-slot
tests and stores are written `== -1` / `= -1` (the attached-lights unit, and the fx unit's slot getter at its line
1619, which would otherwise always see a slot as used). Whole tree: none lost, attached_light_free won.

**D7203 (notes only; match14). What still differs in these units under 8966** (no source shape found):
- The `mov eax, [eax]` of the packet's first int in small handlers (D5812) stays `mov ecx, [eax]` for every void
  form; an int-returning handler (`return mp_summon_owner = *(int *)data;`) reproduces
  mp_handle_summon_allowed exactly but not mp_handle_respawn: open (it would change the type of every packet
  handler).
- Dead argument slots reused for temporaries (mp_handle_adjust_hp, _status_add, _new_path, _obj_update, the
  multispell writers, sound_emitter_play's volume).
- Frame layout of mixed short / float locals (mp_send_obj_update), MP_BUILD_HEADER's counter in ESI
  (mp_send_new_lep), register choices (mp_handle_character_skills, mp_handle_status_cure, NoRlsAllocPool::Grow,
  StringPool::Add, localize_set_string, experience_parse_level, mp_read_spell_targets' base register), store
  scheduling (attached_light_create / _create_b, sound_create_emitter, sound_emitter_reset / _stop).
- sound_emitter_play pushes five arguments for snd_play_looping (0x5463f7-0x546406) although the callee reads four
  and the snd unit's own caller (0x521590) pushes four: the emitter's source called it through a declaration with
  a fifth (unused) parameter. Not changed (header and definition); the merged `add esp, 0x14` of the four play
  calls depends on it.
- A constant in a callee-saved register (`mov ebx, 1` for `push ebx` and the bool stores: ls_ikaemostopint_init),
  decal_place's &d->next kept in EDI, playmenu_frame's key stored back to its argument slot and reloaded,
  effect_stream_finish_entry's join placed after the first case, character_equip_item's case bodies placed after
  the code that follows the switch.

**D7204 (notes only). Markers raised:** functions matched under 8966 in this pass carry "exact (VC6 byte match,
match14; was ...)", including masad 0x4fdcd0, rand_hills01 0x4fa6f0 and temple_inside 0x4f6340 (won by D7201).
Some of their older comments still describe 8168 differences (mp_remote_travel_update's short jump,
item_obj_pickup's shared tails); the marker word is what counts.

**D7000 (vc6-flags.txt; vsdk/unnamed_vfx_mesh_0x562a30.cpp, vsdk/unnamed_poly_split_0x55f110.cpp; match12). FLAGS
CHANGE: both units are /O2 /Ob0.** Their binaries call the header inlines out of line: the vfx mesh unit the vector3 /
matrix / quaternion members, gr::Mode's constructor and the record accessors (its file header says so; D6602 already
measured 3 -> 6 under 8168), the polygon splitter float_min / float_max (0x51faf0 / 0x50d960 from
float_strictly_between) and gr_poly_alloc_clip_vertex (0x546130 from gr_poly_split_edge_vertex). With cl 12.00.8966:
vfx mesh 3 -> 7 of 9 (fx_rec98::init, render, render_billboards, vfx_node_desc::relocate), poly split 0 -> 1 of 5 (the
other four are also closer: the instruction counts of gr_poly_split_edge_vertex and float_strictly_between equal the
binary's only under /Ob0). gr_poly (0x5450c0) calls float_min / float_max / gr_poly_free_clip_verts out of line too,
so its true flags are /Ob0 as well, but its buffer helpers copy a vector3 inline (no operator= call), which /Ob0 turns
into a call: 12 matches under /O2 against 11 under /Ob0 with today's source; left at the default (open).

**D7001 (the units of match12). Source shapes that make cl 12.00.8966 emit the binary's code** (each verified by a byte
match with the 8966 compiler; details in match12 NOTES.md):
- Block layout: 8966 makes the then-block of an `if` the fall-through, so the binary's fall-through decides the
  polarity: `if (n < MAX) { add; return n++; } return -1;` (lobby_add_channel / _user), `if (p) { ...; } else {
  not found }` (chat_cmd_ping, chat_process_command's "/msg", whose `return true` then shares the command block's
  epilogue), `if (state > 2) { if (state == 5) ... } else { level data }` (multi_set_netgame_state), `if (n != 0) {
  ... return; } error;` (File::write is closer), `if (c != 0xff) { type } else { switch }` and `if (key != ESC)
  return key;` (gamecreate_handle_click, closer), `if (len != 0) { ... return; } buf[0] = 0;`
  (File::read_string_buf), and nested `if (spell != KNOCKDOWN) { cast } else if (!(10 || 22)) { grab } else {
  party loop }` (titus_knock_down_party).
- A switch where the binary compares in order: elevator_popup_frame (two cases), cursor_draw (`case 2`, `case 1`,
  `case 0` and `default` with separate identical bodies, in the binary's block order).
- Separate epilogues: switch cases that set the result and `break` to one `return key;` (PollInputEvents: 8966
  duplicates the epilogue per case), each sprintf branch with its own `return out;` (chat_format_entry), `if (f() ==
  0) return def; return v;` instead of `error() ? def : v` (File::read_int16 / read_uint16: the 16-bit `mov ax`),
  `return v ? true : false;` for a char read as bool (File::read_bool), the if-form `if (x > lo && x < hi) return
  true; return false;` for an AL result (float_strictly_between).
- `while (1)` instead of `for (;;)` for a top-tested loop with one call (File::read_string).
- Store order: the global store first when the binary loads the argument first (gamecreate_set_difficulty's
  `gamecreate_difficulty = difficulty;`, mp_send_jn_rqst's `mp_join_target = game;` before the memset,
  mp_session_reset's `mp_local_player = NULL;` before the memset: closer); integer stores written AFTER an inline
  strcpy that the binary interleaves with it (8966 hoists them into the copy, handle_slots_reset), the WNDCLASSEX
  fields in the binary's store order (hInstance, lpszClassName, lpfnWndProc, style, cbSize, hIcon, hIconSm, hCursor,
  lpszMenuName, cbClsExtra, cbWndExtra, hbrBackground: os_create_window), the cursor chain `uv = rgb = alpha =
  vertices` (the last name is stored first: gr_poly_begin / set_texture), gr_poly_end's `in_progress = false` before
  the codes.
- Local copies (D6005) for a global pointer the binary keeps in a register across a store through it:
  gr_poly_buffer_put_byte / put_int (`ubyte *p = gr_poly_buffer_ptr; *p = b; gr_poly_buffer_ptr = p + 1;`),
  gr_poly_buffer_get_vector (`vector3 *p = ...; *out = *p++;`), the uv / alpha cursor index read first, the codes
  byte of a clip vertex read once (gr_poly_clip_plane, closer).
- Element pointers (D6403 / D6702): `ui_button_state *b` / `ui_button_def *d` in gamecreate and dialogue loaders,
  draw loops, click and update_buttons; `gr_poly_vertex_rec *r` (gr_poly_reset_clip_verts, gr_poly_vertex). The
  opposite: pathvis_insert_sorted / _by_side index `pathvis_edge_lists[layer]` each time (the binary keeps the scaled
  index and the head address apart), snow_update and snow_cell::fill index `snow_cells[i][j]` / `flakes[k]` each time
  (the binary reloads them), an index loop for a `jl` loop end (handle_slots_reset).
- Float members assigned one by one instead of the vector3 constructor where a float global is copied with mov
  (snow_cell::update, snow_cell::fill with `t = 12.0f` between the y and z stores).
- `int mask = 1 << bit;` computed once (snow_cell_alloc), `ubyte mask` and the byte index as locals
  (snow_cell_free), `for (bit = 0; bit < 8; idx++, bit++)` with the `idx >= 100` guards as breaks.
- A call argument that is a call through a local (D6005 / D6702): `int music = music_find_by_name("Boss");
  music_play(music, true);` (ls_lpalaceint02_frame, ls_rand_hills01_frame). The same shape is written inline in
  about 20 more level scripts (catacombs, ikaemosl2, ikaemosu, iona_ext2, jadetemple, khosanilab2, liangshan, masad,
  rand_forest01, rand_forestnite1, sewers, temple_inside, templeint2, ...), not part of this pass.
- Header macros instead of hand-written stores (mp_send_gm_qury: MP_BUILD_HEADER / MP_END_PACKET).
- A sum of three squares in the binary's z, x, y order is written `d.y * d.y + d.x * d.x + d.z * d.z` (8966 emits
  the terms in reverse; titus_knock_down_party, as the main session's y / x / z case).
- `ushort len = (ushort)strlen(text);` where the copy count is masked (chat_build_packet).

**D7002 (vsdk/unnamed_gr_poly_0x5450c0.cpp). The command buffer helpers keep the cursor in a register.** put_byte /
put_int store through a local copy and store the advanced pointer once (exact), get_vector copies through a walking
vector3 pointer (exact); put_float, put_vector, put_data and get_data still differ by scheduling: the binary stores the
advanced cursor before the payload (put_vector, put_data) or loads the float before the cursor (put_float); no source
order tried gives it. gr_poly_end, gr_poly_vertex and reset_clip_verts match. Still differs (8966 frame and argument
slot reuse not reproduced by any form tried): set_texture and rgb (the int sum and the tint products are stored into
dead argument slots, the bitmap width into bm's slot), clip_and_draw (nv in list's slot), clip_edge (t in on's slot,
and the out-of-line gr_poly_alloc_clip_vertex of /Ob0), buffer_flush.

**D7003 (game/unnamed_os_0x530b90.cpp, notes on /Od frames under 8966). The /Od frame rule of D6101 holds for
8966.** A probe of 21 int locals compiled with cl 12.00.8966 /Od orders them exactly by the D6101 bucket ((h ^ (h >>
16)) & 15, h = h * 4 + (h >> 4) + c), most recently declared first within a bucket. os_clipboard_get_text matches with
`text` declared before `h`; cpu_identify's locals are renamed (id -> regs, signature -> eax1, brand -> brand_text,
old_class -> saved_pclass, mhz_60 -> rate, old_priority -> prev_priority, tsc_start -> tsc0, ticks -> tsc_diff,
time_start -> begin) and declared in reverse frame order, the CPUID record declared after the output clears (its
zero-init comes there in the binary): every slot now matches; it still differs at the strcat of " w/ MMX", which
the binary expands with the literal's length (two dword moves) and 8966 does not under /O2 or /Od (probe), so
probably another intrinsic switch; PrintSystemInfo has the same strcat expansions.

**D7004 (game/multicontainer.cpp). mp_handle_container_open passes the address of a block-scope copy.** The binary
reads both packet ints before the lookup and stores the container pointer into the dead `data` argument slot right
before interface_set_screen; `{ container *open = c; interface_set_screen(4, &open); }` gives exactly that under 8966
(the function-scope `&c` gives a frame slot). Same behaviour. A reproduction device like D6800's volatile.

**D7005 (vsdk/unnamed_crc32_0x5520d0.cpp). Crc32::init_table's index is signed.** The loop end is `jl` (8966 emits
`jb` for the unsigned index). No match change (the flag store is scheduled before `pop edi` in the binary).

**D7006 (notes only; match12). What 8966 still does not reproduce from any source form tried** (the coordinator's
8966 compiler; these replace the 8168 explanations of D6306 / D6514 / D6603 / D6703 for these functions):
- Dead argument slots reused for locals whose address is taken or that are spilled: gr_poly_set_texture / rgb /
  clip_and_draw / clip_edge, gr_poly_get_section (bw and sw in bm's slot), mp_handle_actv_gm (min_version in data's
  slot), mp_handle_swi_path (the walk bool in data's slot). Only the block-scope copy of D7004 got one.
- The list_remove macro (vsdk.h): the binary takes `&e->prev` once and reloads through it (pathvis_add_edge_node,
  mp_player_alloc); a header change, not done.
- game_is_paused: `mov eax, [game_flags] / shr al, 2 / and al, 1` looks like a bit-field of game_flags (all int
  forms give a 32-bit shift); header question, not done.
- snow_cell::fill_count divides by the 8/3 constant (fdiv [0x581b0c]); 8966 turns every form of `n / 2.6666667f`
  into a multiply by its reciprocal.
- The Titus state reloads its counts after the array stores (titus_boss_load, collect_area_hits,
  update_offense_points): the one-object rule of D6510 / D6511 / D6701 would apply (init_titus timers), a boss_ai.h
  change left for the owner of that header.
- new_dialogue (an /Ob0 unit) passes gr_string_mode to gr_string as a dword without calling gr::Mode's operator:
  gr_string takes gr::Mode by value in the original (the open point of the gr::Mode decision); a gr_font.h change.
- Register choice: Crc32::update, gr_poly_uv / alpha (index / pointer loads), mp_player_disconnect (eax / edx),
  mp_session_reset's loop registers, gamecreate_frame (ebx / edi), mp_handle_deny_jn (D5812 still open under 8966).
- ls_ionaext_frame: the binary normalises the bool result with `sete al / test al, al` before the branch; no form
  of `!f()` / `f() == false` / `? false : true` gives it (an inline wrapper in the original?).
- ls_rand_hills01_goto_khosanistrng: the binary stores level_entry_point after the three strcpys; 8966 hoists the
  store into the first copy whatever its source position.

**D7600 (include/vsdk/geom/vfx.h, vsdk/unnamed_vfx_0x536950.cpp; match17). HEADER CHANGE: vfx_dummy_desc +0x0c is
a union of `ushort flags` and the bitfields `num_frames : 15` / `can_bind : 1`.** vfx_instance::node_can_bind's dummy
case reads the bit as a 16-bit bitfield (`mov ax, word ptr [ecx + 4*eax + 0xc]; shr ax, 0xf`, 0x5377cc); `flags >>
15` promotes to int and gives a 32-bit shift and a bool normalisation. The union keeps `flags` for
unnamed_vfx_dummy_0x55fc70.cpp (`d->flags & 0x7fff`). node_can_bind now matches (VC6 12.00.8966). mingw: 259 files,
0 errors; the 16 other vsdk / fx / levelrender units that include vfx.h compare the same with the old and the new
header (no match gained or lost).

**D7601 (notes only; match17, cl 12.00.8966). Source shapes that make 8966 emit the original code** (each verified
by a byte match, details in the pass notes):
- Switch tables: a boss dispatcher whose binary table spans 0x51..0xc9 with a separate index 0 has the other
  bosses as explicit cases with their own `return false;` / `return true;` (a `break` to the default merges
  them away): boss_cast_special_dispatch, boss_attack_stuns_dispatch, boss_attack_knocks_down_dispatch,
  boss_stage_attack_enabled_dispatch.
- Case bodies are laid out in source order: the vfx tag switches are MESH, LIGHT, EMITTER(_PART), VIF, DUMMY
  (get_node_name, get_tag_name, bind_node, node_can_bind).
- One-bit results: `bool r = field; return r;` per case gives `and al, 1` (node_can_bind; effects_base_is_resident
  and effect_is_cutscene_base, D6306 (7) no longer holds under 8966).
- 8966 orders the terms of a hand-written dot product by the shape of its operands: rows bound to references
  (`const vector3 &r = m.rvec, ...`) give the inlined-dot order of vector3::rotated. The same order cannot be
  reached for unrotated (columns) nor for matrix::determinant (all 36 uniform and 729 grouping permutations
  tried: 8966 canonicalises factor and term order completely for one expression tree).
- Branch layout: `if (flen <= 0.0f) { ... } else normalize();` (matrix::orthogonalized / orthogonalize_b);
  store order as written (matrix::transpose u, r, f; bind_node's bound bit before ci, then tag).
- Temporaries: `&(const vector3 &)vector3(0, 0, 0)` for an argument the binary builds in a temporary slot
  (vector3_rotate_towards_unit); `d = -d; *out += *n * d;` (project_onto_plane); a named vector3 returned
  (skeleton::get_root_velocity); the frame count through a float local keeps two unfolded time factors
  (skeleton::get_root_rotation); a do-while list walk with the cursor and the next pointer as written in
  release_nodes; `if (a || b)` merges for the sete-free return (move_skip_on_level).
- Indexing a global array element directly (`char_infos[p->char_info].x`) instead of through a pointer local
  (character_get_chain_skill).

**D7602 (notes only; match17). Not reproduced under 8966 in these units** (tried, left as they were): matrix43's
three constructors and get_rotation (the last two dwords loaded before the stores: element stores, POD struct
copies of 3 and 9 dwords, float locals, pointer and loop forms all give the plain load/store pairs);
matrix::determinant (D7601); scheduling of integer stores between fcomp and fnstsw (get_axis_angle,
get_major_axis); the FPU stack shape of random_unit_vector / random_direction(_in_cone) and length_quick's
`t + t * 0.5f` (folded to `t * 1.5f` by 8966 in every form tried); register choice of str_hash_nocase_rol6,
boss_threat_find, ds_load_buffer_data; boss_threat_add's `test edi` placement.

**D7603 (notes only; match17). Evidence for declarations, not applied (other units' files).** (1) The unload
callees of ai_state::boss_ai_unload (0x4a04d0) keep ECX: the dispatcher picks EDX for the info pointer exactly as
boss_ai_load does, whose callees are ai_state members (D902): luminar_free_bitmap, tiger_unload, ... are probably
ai_state members too (they do not read ECX; D902 made them free functions for that reason). (2)
boss_dismount_dispatch (0x4a2420) has three separate case bodies calling 0x49c610: three functions folded by the
linker (serpent / tiger / phoenix dismount), which 8966 would otherwise tail-merge. (3) tiger_move_in_range and
giant_salamanka_target_in_reach can be bool (`return d < reach;` keeps tiger's byte match): with both bool,
boss_move_in_range_dispatch loses its neg / sbb conversions (verified in a scratch tree, no match lost in the
tiger / salamanka units); the dispatcher still differs in its Luminar / Murod case.

**D7604 (game/characterinfo.cpp; match17). LOGIC FIX: characterinfo_find_multi_slot never returns past a full slot
table.** With all four multiplayer slots used, the binary calls DebugError and jumps back to it for ever
(0x449adc `jmp 0x449ac5`); the source returned &multi_charinfo_slots[4].info (one past the table). The result is now
a local initialised to NULL (the binary's `xor ebx, ebx`), set to the free slot, with `while (i == 4)
VSDK_ERROR(...)`. Game bug (latent: four slots for at most four players): the error never returns. 64 / 64
instructions; only the order of the two return blocks differs (likely).

**D7700 (match18: script_internal, the ghost rider / machival / murod / phoenix / pyrul / serpent / tiger
boss units, unnamed_fx_0x489c40, new_skill, s3d, unnamed_dialogue_box_0x4d6660). Source shapes that make cl
12.00.8966 emit Sum.exe's code** (each verified by a byte match; details in the match18 NOTES.md):
- Boss AI frames: the choose / select result is a `switch (r) { case 0: go_idle(); return; case 1: ... }`
  (the binary's `sub eax, 0 / dec eax` chain: ghost, machival, phoenix, serpent); where the binary has one
  go_idle call last that falls into the epilogue, the work is nested (`if (r != 0) { ...switch... } else
  go_idle();`, pyrul; murod with both idle paths sharing the call). `case -1: return;` (D7102) is needed
  too. A new target handle goes through a local: `int h = X_target; if (h != target) { target = h;
  owner->combat.target = h; flags |= 0x10; ... }` (ghost, phoenix: the binary stores EAX twice and ORs
  the flags through AL).
- Horizontal distances: `vector3 d = p - q; d.y = 0.0f; if (d.len_sq() > r)` gives the binary's x-then-z
  square order (8966 emits the hand-written dx*dx + dz*dz in z, x order whatever the source order):
  serpent / ghost wander_start, serpent_place_parts. The wander retry loops are `while (tries--)` from 8.
- Loops the binary has unrolled are written out: 8966 keeps `for (i = 1; i <= 16; i++) b[i].flags &= ~1`
  as a loop (skills_update_species_tab: 16 statements; skills_set_page: 6); a switch whose case bodies are
  the same code with constant indices is written out per case (skills_count_page, a local macro).
- Branch layout: the shared `return false` / `return count` last (serpent / tiger seat_rider nest the
  success path; skills_count_in_group as if / else); a found path that jumps past an unconditional failure
  count (`goto picked`, tiger_reposition_update, with the -1 reset as its own if before the state test);
  the if / else of two write_string calls instead of a ternary argument (s3d_read_textured_record, D6900).
- Locals: `const char *name = offense_point_name(i); ca_character *ch = owner->instance->character; tag =
  ch->find_mesh_tag(name);` (machival_boss_load); a quaternion built in a named local then assigned
  (s3d_read_navpoint); the sum stored back into the frame's pos before the by-value call
  (script_action_wait_at: `pos = orient.fvec + c->pos; c->walk_to_point(pos, 100);`); an int slot whose
  byte 0 is set for interface_set_screen (skills_handle_mouse: the binary's [esp + 4], a bool sits at +3);
  dialogue_box::first_visible_line as `(first + i) % 64` with `line[idx]` fields (the pointer form strength
  -reduces differently).
- Store order: fx_new's spell_effect store last; summon_spawn_effect's `d.extent_a = d.extent_b = radius;`.

**D7701 (game/unnamed_phoenix_0x497ac0.cpp). LOGIC FIX (as D6400): phoenix_anim_remap_out copies the
character's own loco anims.** 0x499bb9 `mov edi, [ebx + 4*edi + 0x5ac]` reads through EBX, the argument
c; the mount is only tested for NULL. The source read mount->loco_anims. Now exact.

**D7702 (notes only; match18). Still different under 8966** (forms tried in the match18 NOTES.md): the zero
register (tiger_ai_frame, the four anim_remap_in copies); register swaps (dialogue_box::last_visible_line
ebx / ebp, script_action_wander &pos / this, serpent_part_is_busy_or_asleep's push ebx); the in-memory clamp of s3d_read_color_record
(inline helpers, ternaries, clamp_float: the binary keeps v in the f argument slot); s3d_read_door's
`mov ecx, edi` scheduling (every store order); fx_create_at_camera's negations; pyrul_cast_special's wall /
pushback `return true` (8966 moves it to the end); tiger_choose_attack's 0x1f / 0x1e tests after the 0x21
one and the `mov eax, ecx` return; tiger_path_to_target's out-of-line vector3 constructors.

**D7800 (game/living_entity.cpp, match19). Unoptimised clamps of a variable into itself are if / else.** In the
`#pragma optimize("", off)` part, `x = max(x, 0)` / `min(x, cap)` / `max(0.0f, sk)` is in Sum.exe a compare and
one direct store per branch (`mov [x], 0` and the self-copy `mov eax, [x]; mov [x], eax`); cl 12.00.8966 builds a
?: temporary (or, for `max(int, 0)`, the setle / dec / and idiom) and copies it. Written as the local macros
ASSIGN_MAX / ASSIGN_MIN (`if ((a) > (b)) x = a; else x = b`), keeping the operand order of the min / max they
replace. Other conditionals keep the temporary in the binary (`f = c ? 1 : 0` in add_hp, update_footsteps'
`sounds = ...`, `(short)min(v, 0x7fff)`, XSTR): those stay ternaries (rewriting them lost add_hp and
update_footsteps). A reproduction device like D6101's renames. Won ring_add_xp 0x44ffc0, rings_add_xp_all
0x450420, add_ap 0x450c70.

**D7801 (game/living_entity.cpp). ROUND_TO_STEP is an inline function.** scale_stats_to_level computes
`fld 1.0; fmul 0.5` at run time: an inline function's argument is substituted unfolded at /Od, a macro's literal
is folded (`fadd 0.5`). `round_to_step(x, step)` returns `(float)(int)((x + step * 0.5f) / step) * step` (this
operand order gives the binary's step-first evaluation). The function still differs in `(float)level /
(float)info->level` (the binary: fild, fild, fdivp; every source form tried gives fidiv or a cast store).

**D7802 (game/living_entity.cpp). An inline wrapper around character_find_mesh_tag.** The unoptimised calls on
`instance->character` store the model pointer (and a non-constant name) in compiler temporaries before the push
(`mov [T], edx; push name; mov ecx, [T]`): the code of an inline function expanded at /Od. A file-local
`mesh_tag(ca_character *, const char *)` reproduces it (find_foot_bones 0x45d4c0 won; Character::init's light-bone
call); with a plain local (update_head_look's `ch`) the binary uses it directly, as the wrapper does. Name ours;
the original's wrapper may be a ca_character member.

**D7803 (game/living_entity.cpp, game/combat.cpp, game/unnamed_render_scene_0x481680.cpp,
game/unnamed_script_editor_0x4de0b0.cpp). Source shapes that make cl 12.00.8966 emit Sum.exe's code** (each
verified by a byte match under 8966):
- /Od: a vector built from an inline operator is constructed in place, `vector3 dirn = p - pos;`
  (walk_to_point 0x455e70, is_in_view 0x44f020 with a block-scoped `vector3 d`); D6101's hash order still
  holds under 8966 (probe), so the second vector is named for its bucket (`to_target`).
- /Od: a comparison whose right side is the more complex expression is written the other way round
  (`d.len_sq() - r * r * radius_scale > far_dist * far_dist`, is_in_view).
- Jump-table switches: the case bodies in the order of their table targets (living_combat::update_exchange
  0x466d20: IDLE, ATTACKING, REACTING, HIT, DEFENCE, SPECIAL; play_skill_attack 0x4645f0: kick, trip, push,
  axespin, earthshaker, the self-casts, counter / double, the aimed group); cases that share a table target but
  have their own entries written with their own bodies (apply_hero_model 0x4628a0, the three heavy-model
  species; chain_check_window's states 0, 2-5 as explicit `return -100.0f` cases); a two-value test that the
  binary dispatches with `sub` is a switch (draw_health_bars 0x4848c0 on `e->object->type`;
  apply_net_exchange 0x4685c0 on skill_id and on the defence move). play_skill_attack's default is `m = NULL;
  break;` into the shared null test.
- Separate returns per branch where the binary repeats the tail (`update_attack(); return state == IDLE ? true :
  false;` in each branch of living_combat::update 0x468210; update_ranged 0x467020's REACTING case with its own
  `state = COMBAT_IDLE; boss_clear_current_move` and a default with the same body).
- Bool results: `? true : false` (is_busy 0x468740, update's returns); `return theirs < 0.0f || !(mine <
  theirs);` for EAX (defense_in_time 0x464cb0).
- `return vector3(s * v.x, s * v.y, s * v.z);` for the out-of-line operator* 0x468a90 (return-value elision,
  z evaluated first); `t->orient.fvec.dot(owner->orient.fvec)` written as `owner->orient.fvec.dot(t->...)`.
- Locals: `move_info *m = ...; defense_move = m; if (!m) {...}` (pick_parry 0x4637d0); `ItemType *t =
  equipment[3]->type;` before the cloak branch (apply_hero_model: the binary computes each branch's index);
  `float v` for atof results kept on the FPU and compared with a float constant (ambient / door field callbacks
  0x4eb2e0, 0x4ebcf0); a ternary for the hit-sound material (0x466a80); `const char *file = strchr(..) + 1;`
  before select_by_text (the binary calls strchr before pushing `true`).
- The file_find_files head is a whole file_find_node (as D6900): list_scripts 0x4e7120, list_ambient_sounds
  0x4e50f0 (its sprintf buffer 0x20), and the other list heads of the script editor (the frames of
  fill_action_lists and action_type_allowed now agree; script_create is 4 bytes over).
  `list.next = &list; list.prev = &list;` as two stores (the chained form stores prev first).
- `(c->pos - cur_character->pos).len_sq() < 100.0f` (draw_speaker_icons 0x485810: 8966 orders hand-written
  squares itself).
- A do-while that the binary rotates is `x = next; while (!ok(x)) x = next;` (select_next 0x4e7800).
- show_door_lock 0x4e57a0: buffer 16 bytes, `rec = NULL;` written after the first two calls.

**D7804 (notes only). The /Od `len()` temporary.** Every unoptimised `(float)sqrt` in living_entity.cpp is
followed in the binary by a store to a frame temporary at the bottom of the frame and then the destination
(`fst [T]; fstp [speed]`, get_speed's `fst [T]; fst [s]`); 8966 merges the conversion store with the
assignment for every form of vector3::len tried (cast, implicit conversion, named local, nested inline,
double return, sqrtf, /Op). Not reproduced; get_speed, anim_speed_at_blend, stopping_distance and the two
distance_to_reach functions differ by it.

**D7805 (game/combat.cpp). living_combat::construct calls the GameTimer constructor explicitly** under _MSC_VER
(`((GameTimer *)&timer)->GameTimer::GameTimer();`): a placement new makes VC6 /GX open an EH frame for the
placement delete, which the binary does not have (0x4681b0 won). Other compilers keep the placement new.

**D7806 (game/combat.cpp). chain_window_per_speed is a const float of the unit (.rdata 0x5819ac).** It is read
only by chain_check_window; VC6 loads a namespace-scope const float from memory (probe) where a literal
`* 2.0f` becomes `fadd st, st`. chain_check_window also now has the tail of the window test twice (as the
binary) and explicit cases for states 0, 2-5 (the jump table); it still differs at the SKILL_NONE test.

**D7807 (notes only). Markers lowered.** Functions marked exact from 8168 matches that differ under 8966 are
"likely (an 8168 byte match; differs under 8966, D7807)": equip_bonus 0x4627c0, is_mount 0x462d60, is_rider
0x462d70, ai_choose_special 0x463d00, combat_position_modifier 0x464d00, update_attack 0x466f00,
level_vis_reset 0x487030, script_remove_redundant_npc_navpoints 0x4e0080, list_scripts_filter_cb 0x4e71e0,
door_lock_ok_cb 0x4ec570.

**D7500 (match16 pass: game/level_scripts.cpp, unnamed_avoidance_0x4a6710.cpp, new_inventory.cpp, new_hud.cpp,
vsdk/unnamed_vfile_0x5128b0.cpp, bmpman.cpp, parse.cpp, unnamed_listbox_0x52ca60.cpp). Source shapes that make cl
12.00.8966 (the build of Sum.exe) emit the original code** (each verified by a byte match; details in the pass notes):
- Counted loops over a global array compare the counter (`jl`): `for (k = 0; k < N; k++) { T *p = &arr[k]; ... }`
  instead of a pointer loop (`jb`): avoid_clear_segment_flags, avoid_seglists_clear_refs, avoid_collect_segment_refs,
  inventory_draw, inventory_frame (D6403 still holds).
- An if-chain on two flags ANDed out once: `int a_box = a->flags & 8, b_box = ...; if (a_box && b_box) ..; if (a_box)
  ..; if (b_box) ..` (avoid_obstacles_overlap).
- Float min / max as inline functions with by-value arguments (`static inline float min_f(float a, float b)`): the
  binary keeps the field in a stack temporary (`fst / fcomp st(1)`), the macros reload it (rect2d_add_point).
- `vector2 p(pos->x, pos->z)` through the inline constructor gives the `fld / fstp` copies (avoid_point_blocked;
  closer).
- An if / else of two calls that differ only in a constant argument is one merged call (`push 1 / jmp / push 0`):
  quest_journal_set_entry, savegame_refresh_character_flags (D6405's entry does not apply to 8966).
- 8966 may emit the loads / stores of two independent statements in the opposite order: the source order is chosen
  against the binary (level_script_select: frame before leave).
- Direct global-array reads instead of a pointer local where the binary schedules the loads ahead of the stores
  (level_script_select: `level_script_table[cur_level_index].x` six times).
- Reads before increments: `int stored = *(int *)(data + off); off += 4; return off == stored;` (read_header);
  `int off = data_offset(..) >> 3; float v = value; *(int *)(data + off) = *(int *)&v;` (set_float).
- Re-read the member, not a cached local, where the binary tests the list head again: entries.next in
  ui_listbox::entry_above / entry_at / entry_at_y (snap_scroll_top closer).
- A nested call argument through a local: `int line = e->line_at_y(..); set_vscroll(e->line_top(line), false);`
  (scroll_line_up); the increment inside the argument, `ParseDelimitedString(&list[count++], ..)`
  (ParseStringList); `int count = 0;` after the opening SkipWhitespace calls (ParseFloatList, ParseIntList).
- `while (1) { if (*cursor == '\0') break; ... }` for a loop whose paths all jump back to the top test (SkipSpaces,
  SkipWhitespace; `while (*cursor)` is rotated and duplicates the test).
- Rectangle size arguments through locals `int w = size[res][0]; int h = size[res][1];` before point_in_rect
  (hud_hot_buttons_handle_click; hud_hot_buttons_drag and hud_handle_keys closer).
- A short-valued ternary converted per arm: `x >= 0 ? (int)a : (int)b` (inventory_draw_character).
- The strchr result and the result pointer as two variables (file_chdir); `int err = s->error;` before
  `slot = -1` (File::close, which then matches under /Ox /Ow).
- Operand order of int sums: `inventory_drag_start_thumb_y + (game_mouse_y - track - start_y)` (load order, and the
  single clamp store `last = n - page; if (pos < last) last = pos; scroll = last;` of D6702),
  `index == poly->obstacles[k]`, `int row = scroll + selected;` as a local (inventory_show_item_info).
- Late initialisation: `bool remove = false;` after the early returns (inventory_equip_item: the EBX push is
  shrink-wrapped behind them); the owner / colors stores before the strcpy (ui_listbox::set_tooltip, /Ow).
- Switch case bodies in the binary's order (hud_use_hot_button: default, banish, appraise, pick lock).
- The low byte of a flag word tested after `not`: `(ubyte)~game_camera.flags & 1` (read_camera).

**D7501 (game/new_hud.cpp). hud_member is a macro; the screen argument is a 4-byte block.** (1) Build 8966
evaluates an argument that is a function call before the other arguments; the binary pushes the slot argument of
hot_button_get before it reads hud_selected_portrait, so hud_member() was a macro, not an inline function
(hud_use_hot_button, hud_hot_buttons_handle_click match; hud_draw_tooltips closer). (2) hud_key_inventory,
hud_key_save and hud_use_hot_button pass interface_set_screen a pointer to a flag byte at the start of a 4-byte frame
slot (a lone `bool` lands in the slot's top byte under 8966): written `bool flag[4]; flag[0] = ..;`. The screen code
reads only the first byte; the original type is not known (a small struct fits as well).

**D7502 (flags; notes only, vc6-flags.txt is not changed by this pass). The vsdk library was built without /GF and
with /Ow.** Whole-tree sweep of the snapF tree under 8966: the vsdk units with their vc6-flags.txt lines give 1502
matches, with `/Ow` added 1595, with `/O2` replaced by `/Ox` and `/Ow` added 1606 (parse +8, vfile +6, effect_mem +6,
gr_poly +6, bmpman +5, unnamed_lights +5, ...); three vsdk addresses lose (gr_glide 0x549c80 and unnamed_net
0x517780 under /Ow, sndstream 0x566d70 under /Ox). The game units lose with either change (2690 -> 2402 / 2358), so
they keep /O2. /GF (string pooling, implied by /O2) is what keeps `strcat(p, "\\")` from becoming a word move
(file_make_path, file_make_cd_path, file_change_ext match under /Ox: the "strcat of a short literal" entries of
D6005 / D6603 / D6803); /Ow (assume aliasing across function calls only) lets 8966 keep loads ahead of stores
through pointers (parse's cursor walks, bm_get_data, bm_alloc_entries, File::close; the scheduling entries of D6306 /
D6603). Recommended lines: `vsdk/<unit>.cpp /Ox /Ow /GX /MT`, `/Ox /Ob0 /Ow /GX /MT` where the unit has /Ob0 now.
Functions of this pass that match only under these flags are marked `Confidence: likely (a byte match under
/Ox /Ow, D7502)`.

**D7503 (game/unnamed_avoidance_0x4a6710.cpp). GAME BUG: avoid_area_rebuild's index array has 3 entries.** The
frame is 0x18 bytes with the array at +0xc (`lea ebp, [esp + 0x1c]` after four pushes, dword stores at 0x4a7d5d),
so `int others[3]`, while a polygon merges up to 8 obstacles: removing an obstacle from a polygon of 5 or more writes
the 4th to 7th remaining indices over the saved EDI, ESI, EBP and EBX, which its caller gets back corrupted. The
source declares others[3] now (it had [14], which hid the overflow and gave a 0x44 frame).

**D7504 (game/level_scripts.cpp). Decomp logic fixes.** (1) savegame::automap_labels_reset_level skipped 0x10
bytes for x, z, layer; the record is level, script, x, z, layer (0xc) and the text at +0x14 (savegame_automap_label,
0x34 bytes), as the binary has it (`add edx, 4 / add edx, 0x10` before the 0x20 text). The old source compared the
wrong text and walked 0x38-byte records. (2) savegame::read_info passes NULL as read_header's version pointer
(`push 0` at 0x4d2866), not its own version argument; every caller passes NULL, so the behaviour is unchanged.

**D7505 (include/vsdk/bmpman/bmpman.h, vsdk/bmpman.cpp). HEADER CHANGE: bitmap_entry::reduced is a bool.**
bm_is_reduced returns the byte unnormalised (`lea eax, [entry]; mov al, [eax + 0x40]`), which a `ubyte` field
converted to the bool return does not give (`test / setne`); bm_set_reduced stores its bool argument there. Same
size and offset; only bmpman.cpp reads the field. mingw: 0 errors in all 259 .cpp; the 70 other units that include
bmpman.h give identical VC6 results before and after.

**D7506 (notes only; match16). Still differing under 8966 after this pass, no source form found:**
- The `max(x, 0)` select `setle / dec / and`: Sum.exe ANDs into the value register (`and eax, edx`) in
  inventory_page_up, inventory_destroy_selected_item, inventory_identify_item and inventory_equip_item, where 8966
  ANDs into the mask register for every form tried (macro, inline function, template, ternaries, if, locals); each
  of these functions differs only there. The five inventory_goto_category* functions (mask register) match.
- FileReader::read_int / int16 / uint16 / int8 / uint8 / float: the binary pushes ESI at the entry and keeps the
  buffer in it; 8966 shrink-wraps the push for every form tried, also under /Ox /Ow.
- Register choice and frame-slot order: savegame::get_int (loop registers; set_int / get_float match),
  hud_hot_buttons_drag's id / kind slots (declaration order and local names have no effect under /O2),
  level_item_has, hot_button_get's byte temporary in a dead argument slot, layout_conversation's two pointer slots,
  avoid_circle_contains_point's r in p's slot, the gl_convert_* loop registers.
- A float field copied through `fld / fstp` into a temporary (write_character's lent_max_hp; /Op loses one).
- Markers that say exact but differ under 8966 in these units (62, left as they were; 13 of them match under
  /Ox /Ow, D7502): listed in the match16 notes.

**D7900 (vsdk/unnamed_quaternion_0x5225b0.cpp; vc6-flags.txt; match20). The quaternion unit is /Ox /Ob0 /GX /MT /Ow, and
its two identity constants are defined at the end of the file.** The initialisers call the 4-float constructors out
of line (0x5225c0 `call 0x523d50`, 0x5225f0 `call 0x50f990`), as do normalized / conjugate / log / squad: the unit
was built without inline expansion. Under /Ob0 the out-of-line quaternion(x, y, z, w) COMDAT is compiled first
when the initialisers open the file, and vc6-compare pairs same-named functions in compile order, so 0x522610 was
compared with that COMDAT (the body itself matches, checked with side.mjs). With the two definitions after
cquat::set the pairing is right. Under the library flags of D7502 (/Ox /Ow) the unit has 11 / 32 without /Ob0
and 21 / 32 with it (old source with /Ob0: 16), none lost.

**D7901 (match20; measured with cl 12.00.8966 before D7502 arrived). Source shapes that give the original code**
(each verified by a byte match; details in match20 NOTES.md):
- `memset` of a small int array vs a loop: the binary's `lea edi / mov ecx, n / mov eax, -1 / rep stosd` order is
  what 8966 makes of `for (i = 0; i < 8; i++) a[i] = -1;`; memset sets ecx first (ai_state::init, ai_state::reset:
  the memset also cost reset its zero register and frame).
- A store of a constant written before a memset is scheduled after it as `mov [g], 0` (light_reset_all); written
  after, 8966 reuses the zero EAX.
- Switch layouts: the jump table lists distinct targets in case-value order and the bodies are laid out in source
  order. When every value of the range has a case in the binary (no `sub` of a base, the default index first or
  in the middle), the source lists the values that share the default body as cases of their own with an
  identical body beside `default:` (interface_set_view_clip) or grouped (`case 0: case 1: ... return 0;`,
  status_effect_duration / _modifier). Case bodies follow the binary's order (summon_emerge_update: 0xc2 first;
  status_effect_*: case 8 last).
- Calls to identical-code-folded functions: separate callees in the source keep VC6 from merging the call sites;
  one shared callee (empty_function, ui_return_true) makes 8966 cross-jump them (interface_set_screen,
  interface_mouse_over_panel). See D7903.
- Shared `return false`: one `||` condition (ai_state::can_be_knocked_down), as D7301.
- The inline `vector3::dot` keeps the binary's product order where a written-out sum is canonicalised
  (can_backstab_target `d.dot(back)`; pick_attack_slot closer).
- `? true : false` for a bool from a signed one-bit field (`shr / and al, 1`, light_is_enabled); the binary's
  `mov al, 1` on the true path of `return (over || f()) ? true : false;` (interface_mouse_over_panel).
- Loops: a record pointer with a count-down `for (s = sounds, i = N; i != 0; i--, s++)` gives `dec ecx / jne`
  (snd_init_sounds); `for (i = 0, s = sounds; i < N; i++, s++)` keeps the index compare (snd_level_release).
- Locals: the light pointer in a local (light_set_dynamic); `quaternion r = a->conjugate(); quaternion n =
  r.normalized(); r = n.multiply(*b).log();` (quaternion_ / cquat_log_of_delta: frame 0x40, r shares the
  conjugate's slot); `int iw = w, iz = z, iy = y, ix = x;` (cquat::normalized, as normalize); `pos = c->pos; pos +=
  fvec * 4.0f;` (cmd_summon); struct stores in field order (spell_spawn_hit_fx); `t = 1.0f;` before `flags &= ~2`
  (mode_hiding_frame).
- Branch layout: `if (1.0f - cosom <= EPS) { 0, 1 } else { sin weights }` (quaternion_ / cquat_slerp).
- Constant folding: `level * 4.0f * 0.01f` is folded to one multiply by 0.04 by the front end; `(level * 4.0f) *
  0.01f` keeps the binary's two (status_effect_modifier).

**D7902 (vc6-flags.txt; match20). Superseded by D7502.** /Ow on the vsdk units (alias across calls only) turned
the loads-above-stores differences of lights / snd / quaternion into matches (92 gained, 2 lost over all vsdk
units with /O2 /Ow; game units lose 332 with /Ow: the game was not built with it). D7502's /Ox /Ow is the flag set
used now; this pass's vsdk counts are measured under it.

**D7903 (game/unnamed_interface_0x411270.cpp). Folded screen functions are named apart in the calls.**
interface_set_screen calls the empty leave functions of screens 0, 3, 5, 14-18 and interface_mouse_over_panel
the return-true mouse_in_panel functions of 13 screens; the linker folded each set into one copy (empty_function
0x4f13e0, ui_return_true 0x4063a0), but the binary has one call per case. With one callee 8966 merges the cases
into one call; with separate names both functions match. Declared at file scope in the interface unit (hud_leave,
automap_leave, spells_leave, mainmenu_leave, gamelist_leave, charselect_leave, gamecreate_leave,
levelselect_leave; inventory_ / status_ / saveload_ / journal_ / pxochat_ / options_ / mainmenu_ / gamelist_ /
charselect_ / gamecreate_ / levelselect_ / credits_ / help_mouse_in_panel; names ours). Their units do not define
them yet: a link needs empty definitions in those units (left for their owners).

**D7904 (include/vsdk/gr/unnamed_lights_0x51e070.h; HEADER CHANGE, amends D4006). light::enabled is a signed
`short : 1`, light_set_enabled / light_set_all_enabled take bool.** The stores are `and cl, 1 / movzx cx, cl / shl
ecx, 14 / and dh, -0x41 / or`: under 8966 a bool argument gives the `and cl, 1 / movzx`, and only the signed short
field gives the byte `and dh, -0x41` (ushort, int, uint fields: `and edx, 0xbfff` or worse). D4006's int argument was
read from 8168 output. light_is_enabled keeps its `shr / and al, 1` with `? true : false`. Users: the light unit,
unnamed_vfx_emitter_0x55ff00.cpp (no change, 14 functions compared old / new header), the interface unit
(light_set_all_enabled(false / true), unchanged); mingw over all 259 files: 0 errors. Gains 0x51e100, 0x51e650,
0x51e910.

**D7905 (notes only; match20). Still different under 8966** (forms tried, none matched): the snd volume clamps (A /
B / C / D / E forms, closest `if (v > 0) { if (v >= 1) v = 1; } else v = 0;`); spell_target_value's tail-merged
returns (the body order and the silence fall-through into not_boss are now the binary's, kept);
cast_effect_track_add (pointer / index registers swapped, 1 byte); level_ambient_find_at (the binary repeats the
loop test as `jge / jmp`); level_trigger_point_left_of_line (frame 0x28 against 0x20); level_parse_table (frame
now right with the scoped buffers, the buffer slots still differ); the get_center x sum order and dot slots of
pick_attack_slot / claim_attack_slot; steer_jitter (the limit / decay before the profile label gives the size, the
two temporaries' slots are swapped); quaternion::to_matrix(_b) product order (8966 canonicalises `z * w`);
quaternion::multiply (now to +0xe8, r's store order); spell_effect_obj_set_timer (the binary copies the floats
through the FPU); party_find_next_member (status_character kept in EAX for the NULL return).

**D8000 (notes only; match21; agrees with D7502). The vsdk units were built with /Ow (assume aliasing only across
calls); the game units were not.** Found independently in this pass: with /Ow added to every vsdk line (/O2 base)
the 92 vsdk units go from 1,593 to 1,683 matches (+92, -2: gr_glide.cpp 0x549c80 and unnamed_net 0x517780, both
tuned for the no-/Ow code); /Oa instead of /Ow gives 1,586. The same /Ow sweep over the 167 game units loses
(2,914 -> 2,584) and no game unit gains net. Under /Ow several local-copy shapes that earlier passes used to
imitate the binary (D7001's gr_poly cursor copies, the alpha / uv index locals, the clip codes byte) are no longer
needed: the plain forms (`memcpy(dst, gr_poly_buffer_ptr, n); gr_poly_buffer_ptr += n;`,
`gr_poly_alpha_cursor->pass[gr_poly_cur_texture_index].a = a; gr_poly_alpha_cursor++;`) are what match. The
match21 vsdk units use the coordinator's /Ox /Ow lines (vc6-flags.vsdk-ow.txt).

**D8001 (match21 units). Source shapes that make cl 12.00.8966 emit the original code** (each verified by a byte
match):
- Frame order of initialised locals: pxochat_version_check_frame declares ok, yes_later, cancel, text (the
  binary's loads Ok / Yes / Later / Cancel and the slots cancel 0, ok 4, yes_later 8).
- Switch case bodies in the binary's block order: pxochat_login_frame's result cases 0, 4, 2, 1, 3.
- One call with a selected constant, written as the selection into a local and one call: camera::cycle_mode's
  `int m; if (mode == 0) m = 1; else if (...) m = 2; else if (mode == 2) m = 0; else return false; set_mode(m);
  prev_mode = mode; return true;` gives the binary's three tail copies with `mov eax, C / push eax`.
- A switch with a fall-through for two overlapping range tests: ai_goal_type_fields (`case CAST_LOCATION: |= POS;
  case CAST_SPELL: case CAST_TARGETED: |= SPELL;` gives the `cmp 3 / jl, cmp 4 / jle, cmp 5 / jne` chain).
- A shared `return false` reached from both branches: door_is_open (`if (moving) { ...; if (t > fraction)
  return true; } else if (open) return true; return false;`).
- Direct indexing of a member array instead of an element pointer: ai_state::goal_remove (`goals[slot].type =
  0; ...`), pathvis_edge_pool_init (`pathvis_edge_pool[i].prev = tail;` in an index loop: the binary's `jl`).
- A returned vector3 filled field by field through a local (D7401) also for camera::compute_eye_pos and
  get_target_offset (`vector3 v; v.x = ..; v.z = ..; v.y = ..; return v;`).
- Index loops for the record loops of the vif unit (vif_records_reset, vif_reset_all: `jl`), and the vsdk.h
  plist_append macro for the record ring (vif_record_release; under /Ow).
- The store through the output pointer before the cursor increment: gr_poly_clip_plane's clipped vertices
  `*dst = t; codes...; dst++;` (this also gives the binary's `test bl, cl` operand order).

**D8002 (notes only; match21). Still not reproduced in these units under 8966** (tried, left as they were):
pxochat_page_down (all ternary / if forms give the opposite clamp branch and list in ECX), the three version
popups (8966 hoists the stores common to the -2 path and the rest whatever their order; the binary's `xor ecx,
ecx` zero is not reached), pxochat_init / add_chat_line clamps (`and edx, eax` vs `and eax, edx`),
matrix3_pack_orientation (the first vector2 copy lives in the dead argument slots; block scopes move both or
neither), fx_recb0::get_fade (the `sub esp` is scheduled before the first fld), camera::update_eye_position
(`zoom * elevation` is always emitted elevation first), mp_handle_ai_disable (`mov cx / and ecx, 0xffff` for the
ushort argument), gr_poly_put/get_vector under /Ob0 (vector3::operator= is called; gr_poly is not an /Ob0 unit
whatever its out-of-line float_min / float_max calls suggest, so gr_poly_rgb / set_texture stay apart).

**D8100 (include/engine/ai/boss_ai.h, game/unnamed_titus_0x4c20e0.cpp; match22). HEADER CHANGE: the Titus's state
0x248b3a8-0x248b957 is one object, struct titus_boss titus (as the Pyrul's D6701; listed as open in D7006).**
init_titus_timers (0x4c20f0) constructs the five threat timers and the six state timers in one function (the
one-object rule of D6510), and the binary reloads titus_offense_point_count / titus_area_hit_count after the array
stores (titus_boss_load, titus_collect_area_hits, titus_update_offense_points), which 8966 does for fields of one
object only. Extent as D6511 / D6701: stage first, rock_erupt_fx_handle last (ccmd_titus_combat follows at
0x248b958); the move pointers on both sides stay separate globals. titus_turn_lock and titus_combat_enabled (read by
the ai_combat unit and the level scripts) are fields. Field names ours, LAYOUT_CHECK 0x5b0, the old names are
macros. Won: titus_boss_load, titus_collect_area_hits, titus_update_offense_points (with `pt->pos = pos +
c->centre_offset`, as pyrul / machival). Whole game tree (167 units, cl 8966, unchanged flags): none lost; mingw:
259 files, 0 errors.

**D8101 (game/unnamed_luminar_0x4c8d10.cpp). LOGIC FIX: luminar_ai_release sets both the ready and the ready-hurt
anim from one move (category 0xe, id 0x93).** The binary calls charinfo_find_move once (0x4c9d30, push 0x93) and
stores its anim into anim_ready (+0x5b8) and anim_ready_hurt (+0x5d0); the source looked up 0x95 for the hurt anim.
Now exact. (Whether 0x95 was meant is not knowable; the shipped game uses 0x93 for both.)

**D8102 (notes only; match22, cl 8966). Source shapes that make 8966 emit Sum.exe's code** (each verified by a byte
match; details in the match22 NOTES.md):
- Boss AI frame (titus_ai_frame): the select result as `switch (result) { case 0: end_attack; go_idle; return; case
  1: ... }` and `case -1: return;` in the sub-mode switch (D7700 / D7102).
- A goto into the failure block, `if (play_move(..) == -1) goto fail; h = fx(..); if (h == -1) { fail: end;
  return; } set_mode(..)` (titus_ai_rock_erupt).
- A float local for a damage value computed before a call with pushed int arguments: `float damage = (frand() *
  0.6f + 0.7f) * 120.0f; c->add_hp(-damage, source, 1, false);` (titus_rock_erupt_damage, both loops).
- A dead float kept on the FPU stack: `float off = d - edges; attack_range = reach - off;` (luminar_ai_frame);
  compare operand order `owner->distance_between_edges(..) > radius + 0.19f` (luminar_ai_melee).
- Frame sharing: the effect matrix and vector in an `else { }` block (luminar_beam_hit); `vector3 d; d = a - b;`
  then `travel_dest = vector3(-d.z, 0.0f, d.x) * 1.05f + t->pos;` (the y product folds away).
- Screens: `char page[4]; page[0] = 0;` for the interface_set_screen byte (status_goto_pending_screen,
  journal_goto_inventory, D7200); `if (count <= 0) { simple } else { computed }` (journal list / desc scrollbars,
  D7301); the release / held update template with an element pointer (status_update_buttons); element pointers
  and a 128-byte name buffer in status_load_bitmaps; the clamp through `last` (`pos = max; last = n - page; if (pos
  < last) last = pos; scroll = last;`: status / journal drag_scrollbar, status_page_down with `int pos = scroll`
  read first); the two list clears as a loop (journal_init); a struct copied element by element through a source
  pointer (journal_draw_quest_list's origins); `(int)strlen(name) <= 31` for jg (gamecreate_init).
- `int ok = savegame_state.write(..); return ok == 0;` for `xor / test / sete` (saveload_save_to_slot); the
  operand order of a sum of globals is the reverse of the binary's load order: write `thumb - track - start +
  mouse` to get the binary's mouse-first loads and the right relocation targets (status / saveload drag).
- `(bool)` bit-field tests and one shared `end_cleanup(false)` last (conversation_controller::end).
- vsdk (/Ox /Ob0 /Ow per D7502): an index loop for the lower-casing of a string (localize_load_strings); an error
  variable set in the failure block, as the DirectX SDK's WaveReadFile (wav_read: `fail: err = -1; *read = 0;
  done: return err;`; wav_open closer with `int rc = 0` and `goto ok` / `fail:` blocks); `float head_len =
  d.len() * 0.125f; float len = head_len * 0.5f;` (gr_draw_arrow_3d_between: one expression folds the constants).

**D8103 (notes only; match22). Still different under 8966** (forms tried in the match22 NOTES.md): max(x, 0)'s
AND register in status_page_up (12 forms, as D7305); the multitrade handlers' first loads (D6905 / D7203);
itemtrade_count_party_rows (the binary re-reads party_gold after the row-count store and increments in memory);
gamecreate_draw_player_count / _limits (h / 2 computed twice in the binary; wrong, D9817); titus_ai_select_action's stage-1
block order; luminar_teleport_out's loop exit and luminar_boss_load's second length (closer: `ch` local, float
locals for the pickup reach, `back.len() + swing.len()`); saveload_capture_thumbnail loads camera_fov through the
FPU (an inline float accessor in the original?); packfile_check_header (two loads, then the stores swapped);
register choice in choose_support_spell, mp_charselect page_down / drag_scrollbar, line_talker,
status_handle_click's row loop.

**D8200 (vsdk/unnamed_gadget_0x52bfe0.cpp; match23). LOGIC FIX: ui_gadget::process returns the last child's
result.** With at least one child, the loop exit (0x52c3bc `pop esi; ret 4`) returns EAX from the last
`g->process(key)` call; only a hidden or childless gadget returns key (0x52c3c0 reloads it). The source
ignored the children's results and always returned key. Now `if (!is_hidden()) { g = first_child; if (g) {
int r; for (; g; g = g->next_sibling) r = g->process(key); return r; } } return key;`. Still likely: the
binary reloads key from its argument slot for each child, 8966 caches it in EDI (every loop form tried).

**D8201 (match23). Source shapes that make cl 12.00.8966 emit Sum.exe's code** (vsdk units under D7502's
/Ox /Ow flags; each verified by a byte match, details in the match23 NOTES.md):
- String operator+ (all three): the binary tests the result's `max_len` (`mov eax, [esp + 0x10]`, the first
  field of the local String), not its buffer: `if (r.max_len != 0)` (0x556f70, 0x557070, 0x557180).
- Then-block fall-through (D7001): `if (width <= len + text_len) pad = 1; else pad = ...;`
  (format_indented_column / _b), `if (tok) { ...; return irc_word_buf; } return "";` (irc_get_word),
  `if (!irc_connected) { ... } return 0;` with its own `return 0` in the block (irc_connect),
  `if (state > 2) { if (state == 5) ... } else { level data }` (mp_handle_gm_stat, as multi_set_netgame_state),
  `if (f < 0) f += n; if (f < 0) f = 0; else if (f >= n) f = n - 1;` (vfx_tex_layer::get_frame_bitmap_frac).
- mp_handle_gm_stat initialises level / entry / paused after the MP_IS_CLIENT return, not at declaration.
- Unrotated loops (`jmp` back to a top test) are `while (1)` with breaks: string_to_wide_charmap (`int c =
  src[i]; if ((char)c == 0) break;` gives the binary's movsx-then-`test al, al`), wide_charmap_to_string.
- A strength-reduced index: the curve sampler's `j = i + 1` as its own local (eval_curve34 / 44 / 48 keep
  i + 1 in EDX and index data[j] with it); eval_curve34's switch has `case 0: return 0.0f;` and `case 2:
  return 0.0f;` (the `sub ecx, 0 / je` slot of the binary).
- Store / statement order: ui_gadget::init stores owned before gadget_type; ui_window::set_tooltip stores
  delay, x, y, font and sets the colours before the list append; gr_get_string_size stores *out_h before
  *out_w; gr_close_fonts walks `Font *f = gr_fonts; ...; f++, i++`.
- `strcpy(dest, buf); return dest;` instead of `return strcpy(dest, buf);` (gr_string_truncate_ellipsis, D4905).
- The colour sets are taken as `&ui_window_colors.c0` / `&ui_tooltip_colors.c0` in the /Ob0 window unit (the
  ui.h shim operator is called out of line otherwise, D4202): set_tooltip matches, ui_window::init closer.
- `select(...) ? -1 : 0` / `if (select(..)) return -1; return 0;` for the binary's neg / sbb (irc_connect).

**D8202 (include/vsdk/geom/vfx.h; match23). HEADER CHANGE: vfx_material's blend bit-field is unsigned
short.** vfx_material::reset clears it with a 16-bit `and eax, 0xffc7` (0x564f88); 8966 emits that for an
unsigned short field and `and al, 0xc7` for the signed short (tested). The only other reader,
vfx_instance::setup_render_state (unnamed_vfx_0x536950.cpp), extracts it as `(flags >> 3) & 7` through the
union's ushort, so nothing reads it signed; curve34_mode stays signed (D4608's shl / sar). Whole tree
(259 units, cl 8966, the merged stage's flags with D7502's vsdk lines), old against new header: identical
per-address lists except vfx_material::reset won. mingw: 259 files, 0 errors.

**D8203 (notes only; match23). Still different under 8966** (forms tried in the match23 NOTES.md): dead
argument slots used as temporaries (khosanilab corner_nn / np / pp, gr_classify_sphere_view's FPU stack
copies); register permutations (ui_gadget::send_event / set_parent, skeleton_get_bone_blend_weight,
multi_process_client_status, mp_write_spell_target_point); ui_window::send_event returning the null callback
in EAX (no `xor al, al`); prologue pushes the binary does not shrink-wrap (irc_add_user, ui_window::init);
snow_start's row pointer step scheduled before the inner count; int_to_string_with_commas' counted loop
(the binary divides the trip count by 3 with 0xaaaaaaab); decals_reset's plist_append store order (prev
before next in the binary, 8966 swaps them for the macro and for written-out forms); gr_string's mode
argument passed as a dword without gr::Mode's operator (D7006, a gr_font.h question); cmd_tentacles'
CF_HIDDEN constant held in EBP; vfx_tex_layer::get_frame_bitmap's leftover FPU copy of n / secs * rate.

**D8300 (game/new_buysell.cpp; match24). buysell_goto_inventory's page byte is `char page[4]` with page[0]
written.** The binary stores the byte at the bottom of its dword slot (`mov byte ptr [esp], 0`, `lea eax, [esp]`);
a lone char sits at the top ([esp+3]). The reproduction device of D7200 (open_saveload_screen's `save[4]`), not a
claim about the original declaration. Now exact under cl 12.00.8966.

**D8301 (include/summoner/multi/multi.h; match24). HEADER CHANGE: MP_ADD_FLOAT does not parenthesise its
argument.** cl 12.00.8966 copies `float t = (c->hp);` through `fld / fstp` and `float t = c->hp;` with the integer
moves of Sum.exe (mp_send_player_data 0x4af8c6: `mov edx, [eax+0x5f4] / mov [esp+8], edx / mov esi, [esp+8]`;
tested with the macro text written out, with and without the parentheses, the do-while and the temporary's name:
only the parentheses change the code). The macro is ours (decomp47); `do { float mp_tmp_f = v; ... } while (0)`.
Users: multiplayerdata.cpp (mp_send_player_data now exact), unnamed_mp_join_0x4ab0f0.cpp and
unnamed_swi_0x488230.cpp compare the same with the old and the new macro under 8966 (14/17, 12/14); mingw: 259 files,
0 errors. The same effect may explain other `fld / fstp` float copies where the binary moves integers (D7506's
write_character lent_max_hp): look for a parenthesised expression or macro argument.

**D8302 (match24). Packet fields are read with memcpy into locals.** The small handlers' `mov eax, [esp+4] / mov
ecx, [eax] / mov edx, [eax+4]` (D5812, D7203: every `*(int *)data` form gives `mov ecx, [eax]` or keeps `data` in
another register) is what 8966 emits for `int a, b; memcpy(&a, data, 4); memcpy(&b, data + 4, 4);`; a single field
read into a local and then stored or passed (`int reason; memcpy(&reason, data, 4); mp_join_result = reason;`,
`{ int level; memcpy(&level, data, 4); mp_mark_level_complete(level); }`). The packet builders already use memcpy
(MP_ADD_DATA); the original probably had the matching read macro (FreeSpace's multi code has GET_DATA as
`memcpy(&d, data + offset, sizeof(d)); offset += sizeof(d)`). Now exact: mp_handle_deny_jn 0x470b60,
mp_handle_level_complete 0x4b09c0, mp_handle_difficulty_complete 0x4b0ab0, mp_handle_item_get 0x488c50,
mp_handle_container_close 0x488eb0, mp_handle_leave_game 0x4815c0, mp_handle_door_toggle 0x4aa780,
mp_handle_player_data 0x4af990 (the id / net handle header with an offset, with D8304's tail and clamps). Not
reproduced by it: mp_handle_combat_sequence (the byte fields reloaded as dwords and masked), mp_handle_actv_gm,
mp_handle_join_request (spills into the dead `data` slot). Candidates in other units: mp_handle_respawn and the
other `*(int *)data` handlers of D7203 (not this pass's files).

**D8303 (game/unnamed_input_0x4343a0.cpp; match24). camera_edge_mouse_held is read as a bool bit-field of a copied
input word.** Sum.exe loads the whole word and shifts AL (`mov eax, [esi] / shr al, 4 / and al, 1`, 0x4346e7). Probes
under 8966: `(x >> 4) & 1` and unsigned / int bit-fields give `shr eax`; bool or unsigned char bit-fields read in
place give a byte load (`mov al, byte ptr`); only a bool (or char) bit-field read from a dword COPY (a struct copy,
or a local copy of `*p` reinterpreted) gives the dword load and the AL shift. Written as `unsigned int v = in[0];
((bits *)&v)->left_held` with a file-local bit-field struct (a reproduction device); with the edge test written
`camera_edge_mouse_held & (~in[0] >> 4) & 1` (the binary's `not / shr / and / test cl, 1`) and `float extra =
camera_key_extra_axis();` the function is exact. The same AL shifts in game_is_paused (0x432da0, `shr al, 2`) and
game_render_frame (0x438dc0, `not al / shr al, 5`) read the global game_flags: there 8966 narrows every copy of the
global to a byte load (16 forms and the probes tried), so they still differ; the original game_flags is probably a
struct of bool bit-fields (header question, as D7006).

**D8304 (match24 units). Source shapes that make cl 12.00.8966 emit Sum.exe's code** (each verified by a byte match):
- A statement both arms end with is written once after the if / else; 8966 duplicates it into each arm with the
  binary's unmerged stack pops (`add esp, 4 / push / call / pop ecx`): mp_player_disconnected (`if (host)
  send_left(); else { popup..; if (shown) return; } mp_leave_game(quit);`), mp_handle_player_data (`if (client)
  map(); else send(); owner->flags |= ..;`), party_level_reset (the cur_summon / summon_player stores after the
  leaving / level-change if / else).
- A bool argument chosen by a test is an if / else of two calls with true / false (`push 1 / jmp / push 0` and
  one shared call): ls_lenele1b_action's CheckGold, CheckGold2 and CheckShards (also for the `&&` chain), cmd_slew's
  slew_set_enabled and both console lines; a pointer argument chosen by a test stays a ternary (`save ? save->text
  : NULL`, loadgame_load_cb: the call is duplicated per arm as in the binary).
- Search loops on the parameter: `for (start++; start < count; start++)` gives the binary's `mov esi, [start] /
  inc esi` (party_inventory_find_next_in_category, shop_inventory_find_next_in_category).
- Branch layout: `else if (level != -1) { slot loop } else copy` (game_change_level); `if (event == PRESSED) {
  ..; return true; } return false;` (loadgame_load_cb); `if (i < 4) { stores; return; } for (;;) DebugError(..);`
  (item_paging_find_player_slot); `switch (key) { case KEY_ESC: .. }` for the `dec` test (playmenu_handle_click).
- Store order as the binary: `playmenu_pressed_button = -1;` before `buttons[0].flags |= 2`, and `pressed = i;`
  before `b->flags |= 2` (playmenu_handle_click); MP_END_PACKET before `p->flags |= MP_PF_INFO_SENT`
  (mp_broadcast_new_info); `ch.num_users = num_users;` between the two trim_whitespace calls
  (lobby_parse_channel_list); a field the binary loads before the stores through another pointer is read into a
  local first (`int type = p->type;`, mp_pack_player_info).
- Element pointers `ui_button_state *b` / `ui_button_def *d` (playmenu_load_bitmaps, playmenu_handle_click; closer:
  buysell_handle_click, mp_levelselect_handle_click).
- One local reassigned instead of a second variable for the second living_from_handle (party_level_reset); `char
  last = data[offset++];` before the test (mp_handle_net_info).
- Clamps: a float read into a local and stored back as `h > 0.0f ? h : 0.0f` (the binary's `fcom` with the value
  kept on the FPU stack, mp_handle_player_data's hp); `max(c->ap, (short)0)` for the 16-bit setle / dec / and.
- `float extra = camera_key_extra_axis();` (the int converted at the declaration: the binary's `fild / fld / fmul
  st, st(1)` with the dead copy popped later; camera_process_input).
- `vector3 d; d = a - b;` (worldmap_frame: the x, y, z order; still differs at the fsqrt result's pops).

**D8305 (notes only; match24). Still different under 8966 in this pass's units** (forms tried in the match24
NOTES.md): the `max(x, 0)` AND register (buysell_count_party_items, buysell_page_up, D7104 / D7305); playmenu_frame's
key stored back to its argument slot (D7203); block-scoped slot sharing (playmenu_open_save: save and buttons share
[esp], door_pick_by_mesh: d / open / locked in one dword; every scope form tried); shrink-wrapped pushes
(loadgame_load_cb before D8304's form, mp_levelselect_draw_levels, automap_labels_parse_table); register choice
(buysell_handle_click's `mov bl, 4`, mp_levelselect_handle_click's list loop, experience_parse_level's two induction
pointers, character_use_inventory_item's item load before frand, evil_joseph_ai_frame's submode load, state_title_enter,
pack / build packets); mp_handle_item_drop's matrix temporary copied as nine dwords (matrix has a user copy
constructor and operator=); options_init's reload of options_edit after the struct copy; effect_stream_update's case
layout and effect_stream_start's store / return order; chat_input_handle_key's Esc block position; the cutscene
sample clamps (`fld bound / fld st(1) / fcompp`); stack pops not merged across call runs (ls_lenele1b_init, D7305).

**D8500 (vc6-flags.txt; match26). gr_glide.cpp is a /Ox /Ob0 /Ow unit like the rest of the vsdk library.**
Retest of D7502's exception: under `/Ox /Ob0 /GX /MT /Ow` gr_glide gains gr_glide_tcache_init (0x5479c0) and
glide_record::reset_blocks (0x549880) and lost only glide_record::new_block (0x549c80), which matches again
when the tail link goes through the array (`b->next = blocks[used_tail].next; blocks[used_tail].next = index;`
instead of a `tail` pointer local, D8501). /Ox without /Ow gains nothing. Line now
`vsdk/gr_glide.cpp /Ox /Ob0 /GX /MT /Ow`; 44 -> 48 of 61 with the source changes below, none lost.

**D8501 (match26 units: level_scripts, unnamed_avoidance, unnamed_vfile, gr_opengl, unnamed_ds, gr_glide,
gr_direct3d, bmpman). Source shapes that make cl 12.00.8966 emit the original code** (each verified by a byte
match; details in match26 NOTES.md):
- Retry loops whose every path jumps back to the top test are `while (1) { hr = X->Release(); if (hr >= 0)
  break; sprintf(..); if (tries++ >= 10) break; }`; the `do { } while (tries++ < 10)` form is peeled by 8966
  (ds_channel_release, ds_sound_release).
- A search loop with a result variable and `break` where the binary keeps the result in EAX (0 = not found)
  across the loop: `avoid_link_block *b = NULL; for (..) if (free) { ..; b = &blocks[i]; break; } return b;`
  (avoid_link_block_alloc); `int free_channel = -1; ... free_channel = i; ds_channel_release(i); break; ...
  return free_channel;` (ds_find_free_channel, the binary's EBP = -1).
- Branch layout (then-block = fall-through, D7001): `if (!dsinit) EAXDirectSoundCreate else DirectSoundCreate`
  (ds_init), `if (entry->num_sections != 0) { .. } else { clear }` (gr_glide_tcache_free_entry).
- Switch body order and a shared failure: ds_load_buffer_data lists `case WAVE_FORMAT_ADPCM:` before
  `case WAVE_FORMAT_PCM:` and the ADPCM failure `goto fail;` to the `default:` body's `r = -1` (the binary's
  `jne fail / jmp done`); the PCM failure falls into it.
- Index forms instead of pointer locals where the binary keeps the scaled index (`shl esi, 2` then
  `[esi + base]`): ds_duplicate_buffer (`&ds_channels[channel].pdsb`), glide_record::new_block.
- A pointer local for an array element where the binary adds the base once: ds_estimate_duration
  (`ds_buffer *b = &ds_buffers[buffer];`), ds_load_buffer_data.
- A walking element pointer in the for header: gr_glide_tmapper (`GrVertex *gv = glide_poly_vertices; for (i =
  0; i < nv; i++, gv++)`, the binary's `add edi, 4 / add esi, 0x3c` order).
- Product order: `gv->oow * (gr_glide_texture_scale_v * verts[i]->v)` gives the binary's `fld scale_v / fmul v /
  fmul oow` (all 12 orders and groupings tried; same value as scale * v * oow).
- An explicit `(double)` on a float expression passed to a double parameter makes 8966 store the argument
  before the other statement's integer store, as the binary does: `(int)ceil((double)(frames_per_ms *
  1000.0f))` (bm_load_animation, bm_load_animation_path).
- No `(int)` on an unsigned divisor: `cksize / (bits * 0.125f) / pcm_format->nSamplesPerSec` (the binary's
  int64 conversion with a zero high dword kept in EDX; ds_estimate_duration).
- A local copy of a pointer parameter that the binary loads before the first test: `ubyte *s = src;`
  (gl_convert_888_to_rgb888, as gl_convert_565_to_rgb888 already had).
- The struct-format stores in the FreeSpace order: `wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample,
  nBlockAlign = wBitsPerSample / 8 * nChannels, nAvgBytesPerSec = nSamplesPerSec * nBlockAlign, cbSize`
  (ds_init).
- A play-flags local set by an if: `DWORD play_flags = 0; if (looping == true) play_flags = DSBPLAY_LOOPING;`
  (ds_play_3d; the ternary gives `sete`).
- A `for (p = name; *p; p++) hash = _rotl(hash, 6) ^ (char)tolower(*p);` loop gives the binary's ROL and
  register pair (bm_hash_name; the shift pair / while forms swap ESI and EDI).
- `vector2 p(pos->x, pos->z);` through the inline constructor (avoid_area_contains_point, as D7500's
  avoid_point_blocked).
- A byte test of a ushort flag word: `((ubyte)ob->flags & AVOID_OB_USED)` gives `mov al, [esi]`
  (avoid_point_blocked; a cast on either test is enough).
- Float fields read from a save buffer as integer moves: `*(int *)&x = *(int *)(data + off);`
  (read_automap_labels, read_item_objects: closer, the frame layout still differs).

**D8502 (notes only; match26). Still different under 8966 in these units** (forms tried in match26 NOTES.md):
ds_is_3d_buffer (`shr al, 4 / and al, 1`: casts, ternaries, bool locals, bit-field casts give the dword
shift), ds_load_buffer (the binary spills the index offset and the path pointer and passes `streamed` raw;
ds_load_buffer_data's bool would need snd_load and ds_load_buffer to take bool too, a header change not made),
ds_play (index CSE before the duplicate call), ds_play_streamed (the binary pushes the float priority's bits:
`ds_find_free_channel(*(int *)&priority)` would be the only C form), ds_stream_read_chunk (8966 moves the
`return -1` after acm_convert to the end), ds_create_buffer / ds_fill_buffer / ds_load_buffer_acm
(scheduling, arguments' slots), bm_prefer_vbm (8966 shrink-wraps the ESI / EDI pushes into the TGA branch, as
the FileReader readers of D7506), bm_lock_pcx (`[esi*8 + base]` without the pre-shift), bm_set_reduced (the
last argument evaluated first), bm_load_internal (one more frame slot; `reduced` in memory), the gl_convert
loop-register choices (gl_convert_a8_to_rgba8888 gets the registers with `*d++` per branch but the SIB base /
index of `src[x]` stays swapped), gr_opengl_apply_tint (the sum kept on the FPU with `fst`), avoid spline getters
(8966 puts the shared `return NULL` last), gr_d3d_draw_primitive (8966 preloads the discard flags),
gr_d3d_init / gr_glide_init (the first global stores in the other order whatever the source order),
d3d_enumerate_ddraw_devices (the final `return NULL` placement), savegame::write_character (the float copied
through a stack temporary).

**D8400 (include/vsdk/snd/snd.h, vsdk/unnamed_snd_0x5204c0.cpp, vsdk/unnamed_sound_emitter_0x546150.cpp; HEADER
CHANGE). snd_play_looping takes five arguments, the last unused.** Both callers push five dwords and pop 0x14:
snd_play_3d (0x52157d `push 0` before the looping test, 0x521595 `add esp, 0x14`) and sound_emitter_play
(0x546407, whose four play calls share one `add esp, 0x14` at 0x54640c). The callee reads four (0x520e10). The
declaration, the definition and both calls now have `int unused`. sound_emitter_play matches with this and the
volume through a float local per branch (the binary stores it in the dead emitter argument slot and pushes it from
there). The 18 other units that include snd.h compile to the same status and first offset (901 functions);
unnamed_snd stays 48 / 58 (snd_play_3d two bytes closer). mingw: 0 errors in all 259 files.

**D8401 (vsdk/timer.cpp). timer_get is unoptimised code.** Its frame (0x1c, ebp-based), the `xor eax, eax / mov al,
[timer_initialized] / test eax, eax` test, the 64-bit copies through memory and the `jmp` to one epilogue are /Od
output, the only such function of the unit. It is compiled between `#pragma optimize("", off)` and `on`; the locals
are declared ticks, elapsed, reading, result (all bucket 0 of D6101's name hash) for the binary's slots: result
-4, reading -0xc, elapsed -0x14, ticks -0x1c. Now exact. timer_init: the initialised flag is stored after the tick
count (exact).

**D8402 (vc6-flags.txt). /Ob0 for four more vsdk units.** Tested per unit (8966, the D7502 flags with and without
/Ob0): unnamed_ui_element_0x542cd0 (gains 0x542cd0), console (0x50acd0), unnamed_inetgetfile_0x51b760 (0x51b960),
unnamed_vfx_dummy_0x55fc70 (0x55fc90), each without a loss. Lines now `/Ox /Ob0 /GX /MT /Ow`. Removing /Ob0 from
the units that have it loses in every one tested (sound_emitter 3, d3d_tcache 5, ca_mesh 2, vfx_channel 10, bezier
9, linefile 2, attached_lights 7, poly_clip 4, eax 1, anim_track 10); adding it loses in ui_slider, ui_button,
ui_static and ui_container.

**D8403 (include/vsdk/ui/ui_container.h, include/vsdk/ui/window.h, vsdk/unnamed_ui_manager_0x52f570.cpp; HEADER
CHANGE, amends D4206). ui_container::get_mask_buffer and ui_window::get_mask_data are not inline.** ui_draw calls
0x530b70 (0x52ffd0) and 0x530b80 (0x53001e) while it inlines gr::Mode's conversion, so the unit is not /Ob0 and the
two getters were ordinary functions; they sit at the end of the unit. Declared in the classes, defined at the end of
the UI manager file with markers: ui_draw, 0x530b70 and 0x530b80 match. The only callers are in ui_draw; the nine
other units that include the headers give the same results (292 functions); mingw 0 errors.

**D8404 (match25; cl 12.00.8966). Source shapes that give the original code** (each verified by a byte match):
- Store order around an inline strcpy / memcpy: the stores 8966 schedules into the string copy are written in the
  source before it when they go to the new object (the tooltip owner, then strcpy, then delay before x: the five
  set_tooltip functions of ui_slider, ui_button, ui_inputbox, ui_checkbox, ui_static; ui_container's is delay, x,
  y, font) and after it when they go to `this` (NoRlsAllocPool::Grow: memcpy, then last_allocation = p). For
  consecutive independent field stores 8966 emits the second of a written pair first.
- Index loops: `((ubyte *)buf)[n] ^= key` (lfsr_cipher::crypt) and `rows[i] = &rgb[i * w * 3]`
  (jpeg_write_file): the pointer is then loaded after the count test, as in the binary.
- An if / else-if chain whose branches each end in the same calls (level scripts): 8966 tail-merges them and the
  pending stack pops (one `add esp` at the end): ls_lenele1d_init, ls_lenele1c_init (v31 per branch),
  ls_lenele1aa_init (the eight _v3 calls in both branches), ls_sewer_init (v11..v52 written in full per branch).
- Do / while (true) with inner returns for the binary's separate epilogues (ftp_get::read_data_channel).
- `delta = now; if (now < 0) delta = ...;` (FrameDone); `int total = (int)t; secs = total % 60; minutes = total /
  60;` (time_format_hms); `size = (size + 15) & ~15; if (offset + size > this->size)` (allocate_in_mempool);
  `int want = value != 0;` after the bounds (BitArray::all_equal); `(flags & X) ? true : false` (is_latched);
  `memcpy(code, recv_buffer, 3)` (read_server_reply); `fds.fd_array[0]` before `fd_count` (read_control_channel);
  `carry = 0; if (sum & 1) carry = 1;` and the expiry read into a local before time() (build_stamp_check).
- ui_slider_base::process: the press test with the marker rectangle first and the page-down case last, drag_state
  read directly (no state copy), `len = w - marker_size`, the rounding as `if (num < 0) num -= len / 2; else num +=
  len / 2;`.
- Frame sizes: read_server_reply's code buffer is 5 to 8 bytes, time_format_hms's part buffer 9 to 12 (12 taken).

**D8405 (notes only; match25). Still different under 8966** (forms tried in match25 NOTES.md): fast_inv_sqrt /
fast_sqrt / fast_sqrt_refined (EAX / ECX swapped for every expression form); key_flush (8966 turns the zeroing
loops into rep stosd, also pointer and while loops; volatile arrays lose other functions) and key_clear (the lock
push scheduled between the stores); key_remap_layout (layout kept in EAX, code reloaded per branch);
cmdline_take_argument (now 6 lines: is_done stored after the inline strcpy, the binary inside it); Crc32::update
(the table word XORed into the shifted value); StringPool::Add (register swap); init_char_types
(`je ret / jmp body` prologue); eax_init (the success block last); sound_emitter_delete (push before the enabled
test, D7506); mem_report_line (`65 - strlen(text) - len` order canonicalised); movie_play (the File object at
[esp], below the path); ftp_get::ftp_get (the binary has no zero register); Player::is_leader (`shr al, 3`);
ls_ionaext_frame (the negated bool materialised with sete); ls_ikaemostopint_init (the constant 1 in EBX);
mp_handle_swi_path (the walk flag through the data slot); ui_process (register choice of the restore compares);
ui_static::draw (62 bytes short); build_stamp_check (only the return 3 through EDX). The pickup / assess /
item_info page_up functions are D7506's max(x, 0) case. Markers that said exact and differ were lowered to likely
(10, listed in the notes).

**D8406 (game/level_script_sewers.cpp, notes only). avoid_spline_def's vector2 arrays.** ls_sewer_create_avoid_spline
(0x4ee3c0) and script_build_avoid_spline (0x4ddc60) build a stack avoid_spline_def without calling the vector
constructor iterator (0x404880); with p0 / p1 as vector2 arrays (user-declared default constructor) 8966 calls it
through avoid_spline_def's implicit constructor. In the original the two arrays had no constructor (POD floats or
a vector2 without constructors in that build). Not changed: it is a header change across the avoidance unit,
khosani_lab, script_internal and sewers.

**D8600 (game/new_hud.cpp). LOGIC FIX: hud_button_action's panel cases are the Press entries 43, 46, 49, 52,
55, 58 and 61.** The byte table 0x40f0e8 (button - 21, 0..40) sends indices 22 / 25 (buttons 43 / 46) to the
combat-mode body, 28 to status, 31 to spells, 34 to journal, 37 to inventory and 40 (button 61) to skills; the
compare is `cmp eax, 0x28`. The source had 42 ... 60, so the panel icons (hud_update_buttons passes the hovered
Hilite entry + 1, the Press entry) never ran their action. Now exact (with case 32's body after 39, the binary's
block order).

**D8601 (match27, second 8966 pass over pathfinding, characterinfo, the interface unit, new_hud,
level_scripts_common, new_inventory, spells, new_pxochat). Source shapes that make cl 12.00.8966 emit Sum.exe's
code** (each verified by a byte match; details in the match27 NOTES.md):
- Constant stores written before a memset are scheduled after it with the memset's zero EAX (`xor eax, eax`
  before `mov ecx, n`): pathfinding_init's pointer NULLs before the two memsets (D7901 measured the opposite
  case: one store written before a memset becomes an immediate store).
- Floats copied with fld / fstp come from an inline with float parameters: the vector2(float, float)
  constructor for named locals (`vector2 a(w->a->x, w->a->y);`, pf_wall_normal), a file-local
  `set_xz(float *, float, float)` / `pf_set_xz(vector2 *, float, float)` for struct fields
  (spell_effect_obj_set_timer, pf_slide_move). Plain member copies are integer moves. The original's
  avoid_area_desc and pf_sweep probably have vector2 members set through such an inline (not changed: headers).
- vector2 operators where the binary has a temporary whose y goes through a frame slot and EAX: `d = b - a`,
  `n = s.hit - *s.last_corner`, `s.to = d + s.hit - proj + push`, `proj = n.dot(d) * n`,
  `centre = 0.5f * (a + b)` (pf_slide_move, spell_place_ground_effect).
- Jump tables: the values that share the default body as their own cases (`case 1:` and `case 6:` each with
  the default's body: pxochat_draw_chat_lines' 0..5 table, as D7901); case bodies in the binary's block order
  (pxochat_button_action: 14, 16, 29, 31, 2 ... 12, 18, 20, 22, 26). In a compare-tree switch the source order
  of the bodies had no effect (hud_handle_keys).
- Each case with the whole tail (`select(n); key_status(); sound(); break;`) where the binary pushes the case's
  constant and jumps to one shared call with merged stack pops (hud_portrait_action).
- A selector local for duplicated table lookups: `frame = 2 / (mode != 3) / 0 / 3; bm = icons[id][frame];`
  gives the binary's `mov eax, k; lea edx, [eax + 4*ecx]` in every branch (8966 copies the small join block
  into each predecessor): hud_draw_hot_buttons.
- Frames: two block-scope `char name[64]` (pxochat_draw_chat_lines, frame 0x94).
- Single exits: `e = NULL; for (..) if (free) { e = ..; ...; break; } return e;` (spell_effect_alloc, the
  `xor eax, eax` before the loop); `if (mp hero) {...} else info = &char_infos[index]; return info;`
  (characterinfo_find_multi_slot: the record return laid out before the slot return).
- An integer store written before an inline strcpy that the binary schedules into the copy (lc_entry_add's
  flags byte), as D6900 / D7001 but in the other direction for this function.
- A switch for an `a == 4 || a == 0x19 / else if (a == 0xb)` chain the binary compares in value order (4, 0xb,
  0x19), and an if / else of two calls for the selected constant (script_object_receive_go).

**D8602 (game/level_scripts_common.cpp). LOGIC FIX + GAME BUG: script_object_receive_go tests only the current
action.** The action pointer is taken once (0x4ce280-0x4ce283 `lea ebp, [esi + 8*eax + 4]` from cur_action,
before the loop) and never advanced; the loop end (0x4ce2b8-0x4ce2c1) increments only the index. So the
"wait for go" handling looks at actions[cur_action] num_actions - cur_action times; a waiting action after the
current one is never found. The source walked the actions with actions[i]; it is now
`for (i = so->cur_action, a = &so->actions[i]; i < so->num_actions; i++)` (the original's missing `a++`).
Exact.

**D8603 (game/unnamed_interface_0x411270.cpp). The choice popup's and the message box's .bss state are objects**
(file-local structs with macros for the old names; struct / field names ours). ui_choice_popup_show re-reads
ui_choice_popup_count after every store into the choice table (0x413706-0x413717) and ui_message_box_show
re-reads the button count after each label copy, which 8966 does for fields of one object only (D7201 /
D8100). Extent: ui_choice_popup_title (0x5bd518) to ui_choice_popup_hover (0x5bd778); ui_message_box_title
(0x5bd370) to ui_message_box_hover_button (0x5bd500). ui_choice_popup_flags (0x5bd77c) and
ui_message_box_flags (0x5bd504) are probably the next fields, but interface.h declares them (new_status.cpp
reads the first), so they stay separate globals. Both show functions are closer (first difference 0xc -> 0xb3
and 0x86 -> 0x12e); no function of the unit lost.

**D8604 (notes only; a header proposal, not applied). character_instance::tint[3] is three separate bytes.**
statscreen_clone_instance (0x412f20) is the implicit operator= of character_instance; Sum.exe copies +0xc9,
+0xca, +0xcb as three byte moves (0x4130fe-0x413120), which VC6 emits for three ubyte members and a byte loop
for `ubyte tint[3]`. With `ubyte tint_r, tint_g, tint_b;` the function matches (tested in the match27 tree;
an anonymous union with the array keeps the loop). Not applied: vsdk/character_instance.cpp (constructor,
set_tint) and vsdk/unnamed_ca_render_0x520420.cpp index tint[0..2] and belong to other passes; whoever owns
them can rename (gr_light_desc::tint, a separate member, is not affected).

**D8605 (notes only; match27). Still different under 8966 after this pass** (forms tried in the NOTES):
pf_read_edges' `lea ebp, [eax]` (for / do / while / unsigned / local-count forms); the zero register shared
between a compare with 0 and later stores (`xor ecx, ecx; cmp eax, ecx`) in the three pxochat version popups and
inventory_popup_result (switch, case 4, early-return forms); the max(x, 0) AND register (pxochat_add_chat_line,
ui_tooltip_set, inventory_page_up ...); hud_drag_icon_begin's `or al, 1 / and eax, 1` (store orders, signed /
unsigned fields); hud_handle_keys' shared sound tail (kept at ESC in the binary, at the last copy by 8966) and
case order; summon_stash_for_cutscene's process() arguments (the nested call evaluated before the constant
pushes in the binary); character_can_use_summon (8966 turns every `if (x) return true; return false;` into
setne); interface_init_global's FPU register variable (screen_w in the binary, base_w in ours); frame slot
order of hud_draw_tooltips (declaration order has no effect); inventory_drag_to_equip's cross-jumped case 6/8
-> 7 block; pf_sweep_test_wall (needs the vector2 operator rewrite of the whole function, frame 0x48).
Markers lowered to likely (exact before, differ under 8966): 0x447e80, 0x4caed0, 0x4cd330, 0x416290, 0x4165c0,
0x49cc50, 0x47e830, 0x413200, 0x413290, 0x4134b0, 0x413c70, 0x414110. Raised: cmd_levelchars 0x449c00 (already a
match).

**D8604 applied (main session, 2026-10-16).** `character_instance::tint[3]` is now `ubyte tint_r, tint_g, tint_b`
(include/vsdk/ca/character_instance.h); the constructor and set_tint in vsdk/character_instance.cpp and the copy
into the light record in vsdk/unnamed_ca_render_0x520420.cpp name the members. Whole-tree comparison under 8966,
stage before and after: statscreen_clone_instance (0x412f20) matches, nothing else changes.

**D8700 (game/living_entity.cpp, match28, cl 8966). Optimised tail: source shapes that give Sum.exe's code** (each
verified by a byte match):
- equip_bonus 0x4627c0: the item type through a block-scope local, `ItemType *t = equipment[i]->type;` before
  the inner loop (the binary's ECX / EDX register choice).
- is_mount 0x462d60 / is_rider 0x462d70: `unsigned int f = mount_flags; return ((ubyte)f & 2) ? true : false;`
  gives the binary's dword load with `shr al, 1 / and al, 1` (a byte member gives a byte load; the plain int
  shift gives `shr eax`). A reproduction device; the original may have read a one-byte bit-field copy.
- set_underwear_textures 0x462a20: the `defaults` table declared before `slots` (8966 then schedules the
  initialiser stores as the binary does).
- set_armour_alt_texture 0x462c80: one else-if chain `if (on && !(f)) set; else if (!on && f) clear; else if
  (!force) return;` shares the force test (the nested form gives it twice).

**D8701 (game/unnamed_script_editor_0x4de0b0.cpp). cl 8966 shrink-wraps the register saves: EBX / ESI / EDI
are pushed only on the path that needs them when that region has ONE exit.** Probes: a block with two
`return` statements makes 8966 push the registers at the function entry; the same block ending in one
`return` (a `goto` to it, or the work nested so that both paths meet) gets the pushes inside the block, and
8966 then duplicates the epilogue per exit itself (the binary's repeated `pop / mov al, 1 / pop / ret`). Many
matched functions already show it (door_toggle_state 0x476ee0 pushes EBX after its two early returns). Used:
- SE_COMPLETE (the type-ahead): `goto complete_done;` after the found statement, `complete_done: return true;`
  at the end, and the caller's function-scope `ui_listbox_entry *first = NULL;` (the binary clears EBP in the
  prologue): script_name_cb 0x4eaf70, ambient_name_cb 0x4eb1c0, door_key_cb 0x4ec000, door_item_cb
  0x4ec100, level_item_cb 0x4eca00. door_sound_inputbox_cb 0x4ebb20 still differs (the binary keeps the
  list pointer in ESI from before the region, pushed at entry).
- on_delete 0x4e6ad0: `if (pressed) { if (navpoint) { if (!in_use) { ...; switch { ... break; } sel = NONE; } }
  else if (character) { switch { case 7: status; break; ... } } return true; } return false;` and the hover
  test against script_editor_sel.object before `np` is taken (needs D8703 for the record copies).
- door_lock_ok_cb 0x4ec570: `rec = NULL;` after the early return (not at the declaration).
- script_remove_redundant_npc_navpoints 0x4e0080: `np = &navpoints[i];` inside an index loop (the pointer
  initialised in the for header is set before the count test, which pins EBP at the entry).
- list_scripts_filter_cb 0x4e71e0: the cutscene test as a positive `if (has_suffix) { TextFile tf; ... }
  return false;`, and `name` 0x24 bytes (the frame is 0xb4; 0x40 gives 0xd0).

**D8702 (game/unnamed_script_editor_0x4de0b0.cpp). LOGIC FIX: script_editor_default_action_navpoint 0x4e8cf0.**
When the search loop over the level navpoints runs to its end, the binary returns the last level navpoint
(0x4e8d41 returns EAX, the hoisted &navpoints[count - 1]), not NULL; only an empty table returns NULL (ESI).
The loop reads address 0x28 (a NULL pointer) on every pass, so the path is not reachable without an access
violation; the source now returns &navpoints[navpoint_count - 1] after the loop too. Still differs (the
binary's NULL lives in ESI).

**D8703 (include/levelscripts/level_scripts.h). HEADER CHANGE: navpoint +0x24 / +0x28 are plain members.** The
anonymous unions `{ int _unk24; float ground_height; }` and `{ int _unk28; int type; }` make VC6's implicit
struct copy copy every union member (the dwords at +0x24 and +0x28 twice); script_editor_on_delete's record
moves (0x4e6b70, 0x4e6c38) copy each dword once. Now `float ground_height;` and `int type;`. Callers changed:
level_scripts.cpp 3086, script_internal.cpp 150 / 234, unnamed_script_editor_0x4de0b0.cpp 2746 / 2783 / 3138
(`np->_unk24 = 0` -> `np->ground_height = 0`), level_scripts_common.cpp 373 (`np->_unk28` -> `np->type`). All
259 .cpp pass mingw -fsyntax-only; the whole-tree VC6 sweep loses nothing (4,783 -> 4,803, the 20 wins of
this pass; level_scripts, script_internal, level_scripts_common unchanged at 85, 17, 65).

**D8704 (game/unnamed_render_scene_0x481680.cpp). render_scene shapes.**
- character_draw_overhead_bitmap 0x481c40: the fallback tag lookup as its own `if (tag < 0) tag =
  find("Head");` then `if (tag >= 0) { bone } else { centre }` (the `||` with an assignment gives the other
  block order).
- add_effect_entry 0x4841e0 (closer): the culling as `if (owner > 0) { ...; if (visible owner) { if (!drawn)
  return false; } else if (cull_sphere) return false; } else if (cull_sphere) return false;` (the two
  gr_cull_sphere calls of the binary).

**D8705 (game/unnamed_render_scene_0x481680.cpp). The draw-entry allocator is a macro that yields the success.**
Sum.exe stores a new entry into its list right after the pool test, with no NULL test of the pointer (five
sites, e.g. 0x48368d, 0x4842c4); an inline function returning NULL on overflow makes 8966 test the pointer
again (probe). `LEVELRENDER_ALLOC(e, type)` is `(e = (type *)cursor, cursor += sizeof(type), cursor - pool
<= LEVELRENDER_POOL_SIZE)`; add_effect_entry writes `if (ALLOC) { push } else e = NULL; if (!e) return
false;` (the binary's NULL test after the list push). No function matched by it alone (add_items /
add_containers / add_lobj keep their FPU-order differences), none lost.

**D8706 (game/combat.cpp). LOGIC FIX: living_combat::update_attack 0x466f00.** The jump table 0x467008 sends
states 2 (COMBAT_HIT) and 3 (COMBAT_DEFENCE) to 0x466fff (`state = COMBAT_IDLE`) directly; only states above 3
(the default, 0x466ff0) wait for move trigger 6. The decomp had HIT / DEFENCE in the default. Now `case
COMBAT_HIT: case COMBAT_DEFENCE: break;`: byte match.

**D8707 (game/living_entity.cpp, the /Od part). Frames and returns.**
- The D6101 bucket renames work under 8966 as under 8168: update_footsteps 0x45d690 (`tag_value`, the int
  below w), update_status_effects 0x45b6e0 (11 locals renamed and tie-ordered: invis, n_active, some, s,
  petrified, v, res, res_magic, src, aura, me), perform_spell_cast 0x45da60 (spell_fx, fx_id, mi, anim,
  recovery, fx_base, fx_obj, vi). The binary's frames of these functions are in the declaration order of the
  first decomp, so the original compiler's order may be plain declaration order (D6101's open point); 8966
  here gives the hash order, so the renames stay a reproduction device.
- /Od does no jump threading: a `jmp` straight to the epilogue from inside a nested if / else is a `return;`
  (set_locomotion_speed 0x45a6e0's first case).
- A /Od switch with only `case 0: case 2:` and no default dispatches `cmp / je, cmp / je, jmp past the
  switch`: update_status_effects' expiry code sits inside that case (a `default: continue;` adds a jump).
- `if ((v = (1.0f - res_magic) * v) < 0.0f) v = v; else v = 0.0f;` for the binary's `fst [v] / fcomp`
  without a reload (a separate assignment reloads v).
- A 23-bit field read unoptimised is `shr / and 0x7fffff`: perform_spell_cast writes `(vi->id_bits >> 9) &
  0x7fffff` (the optimised units read `id_bits >> 9`; vfx_instance::id_bits is likely a bit-field).
- set_locomotion_speed's frame is 0x1c with 16 bytes nobody uses: `float spare[4];` (any 16 bytes in a bucket
  above d's).

**D8708 (notes only; match28). Still different** (forms tried in match28 NOTES.md): get_resistance 0x4626b0 (the
binary keeps 8 bytes of frame with the curse on the FPU; with the curse as an else-branch the frame is 4),
say 0x461dc0 (now quoted[256], best = 100 before the first loop, the record stores through the index: differs
only in the strncat loop's load order), add_effect_entry's depth (8966 emits fvec.z * dz first for every term
order; fvec.dot(d) gives the x, z, y order but keeps dx alive), chain_check_window 0x466ae0 (8966 keeps the
second branch's copy of the merged learn tail and cross-jumps the first NONE return into it; a label in the
first branch disturbs the earlier `return 100.0f` merges), the matrix assignments whose rows call vector3's
copy constructor / operator= out of line in the binary (focus_selection 0x4e75c0, push_attachment 0x483430,
combat_get_weapon_tag_position 0x4665f0: 8966 expands them fully here; an inlining-budget question for the
vector.h / matrix.h inline members, not touched), edit_button_cb 0x4e8020 (one exit gets the late pushes, but
the binary places the post-switch tail right after the WANDER case), resolve_hit 0x467df0 (no_flinch in EDI
in the binary), load_state 0x458800 (its function-scope frame is in declaration order too, but the LE_GET
macros use i / b / h / ofs, so the renames reach into the macros: not done).

**D8900 (vsdk/unnamed_gr_3d_0x50d9b0.cpp; match30). gr_light_desc's constructor calls vector3() on +0x0c
explicitly.** 0x50f3e0 runs `vector3__vector3_default` on this + 0xc before reset (the record has a vector3
at +0xc that gr.h splits into _unk0c / appearance / f14, because the callers store a pointer at +0x10).
Under _MSC_VER `((vector3 *)&_unk0c)->vector3::vector3();` (as D7805); the /Ob0 unit calls it out of line
as the binary does. Other compilers skip it (the constructor does nothing). No header change. Now exact.

**D8901 (game/gametarget.cpp; match30). LOGIC FIX: character_can_talk_to looks up the shopkeeper by
npc->name itself.** In the network branch the binary pushes ECX (0x43a5d2), which holds npc + 0x2f4 from the
`lea ecx, [edi + 0x2f4]` of the name test; the info-name fallback (EAX) goes only to the single-player
speaker test. The source passed the fallback to both. Behaviour in Sum.exe: a multiplayer NPC with an empty
name is looked up as "" (no shopkeeper). Still likely (the d.z / d.x square sum keeps both components on
the FPU in the binary; no source form found).

**D8902 (match30 units). Source shapes that make cl 12.00.8966 emit Sum.exe's code** (each verified by a
byte match; details in match30 NOTES.md):
- Calls to identical-code-folded functions named apart (D7903): boss_dismount_dispatch calls
  serpent_dismount / tiger_dismount / phoenix_dismount (declared in boss_ai.cpp; all three are
  boss_dismount 0x49c610 in the binary, a link needs the two extra definitions, left to the owners).
- Indexing instead of a walking pointer where the binary biases its pointer differently: boss_threat_find
  (`table[i].handle`), ai_state::save (`goals[i].x`, no element pointer).
- Parenthesised float copies give the binary's `fld / fstp / mov` (D8301 in reverse): `f = (g->time);`
  in ai_state::save. The name-hash argument chosen by a test is an if / else of two calls (D8304).
- /Ob0 vsdk unit: a vector difference used once by an out-of-line member goes into a named local first
  (`vector3 d = *p - gr_view_pos; *(vector3 *)out = d.rotated(gr_view_matrix);`: gr_rotate_vertex,
  gr_rotate_vertices), the opposite of D7401's /O2 rule.
- The x term computed into the destination field (`dir.x = (sx - xmul) / xmul; ... dir.x = scale.z *
  dir.x / scale.x;`: gr_screen_to_world_ray).
- `if (ret == SOCKET_ERROR) return ret; return ret;` gives the binary's `cmp eax, -1 / jne / or eax, eax`
  (nw_can_write; as mp_local_handle_from_net 0x475280), with the timeout stored before FD_ZERO / FD_SET.
- A sparse switch the binary compares value by value is an if-chain with `||` (ai_state::think's travel
  sub-mode: 0, 3 / 5 / 4, 6 / 7, 0x2b); `int t_party` declared before `int t_goals` gives the registers.
- The screen argument passed through a block-scope copy (D7004): object_open_pickup.
- Float min / max through by-value inline functions (D7500) where the binary keeps the value on the FPU
  (`fcom`, `fstp st(0); fld c` when clamped): camera::apply_input_deltas, world_pick_ground_point (the
  decal scale and the fov); with the eye distance as `(eye_pos - *pos).len()` (D7401) and the lobj index
  in a local before lobj_face_get_surface.

**D8903 (notes only; match30). Still different under 8966** (forms tried in match30 NOTES.md): the snd
volume clamps (empty-then, nested, goto, ternary, comma and do-while forms all canonicalise to the same
layout); nw_init_networking's block order (the slow branch's continuation placed before the 56k / fast
tests; goto into the slow block does not move it); camera::set_mode (the binary compares [esi] in memory
and reloads mode after the flag stores) and update_orientation's ring index (`dec / test / jge` against
8966's `dec / jns` for every form); rider_unlink_mount's register for the flags temporary;
consider_life_drain's unshrink-wrapped `push edi`; boss_target_list_build's members pointer in the dead
`list` slot; vector2_dist_sq_to_line's results in argument slots; cquat log / exp keeping fy / fz as
spilled floats; gr_cull_bounding_box_transformed's plane local without a constructor call.
Closer, kept: ai_state::load (indexed goals and an owner local: 274 / 274 bytes, differs at +0x35 in the
rotation of the load registers), shadow_create_blob_bitmap (the palette as an indexed loop with an
unsigned counter, x unsigned: to +0x40), world_mouse_frame (the target distance as an unnamed difference:
to +0x3c), character_can_talk_to (D8901).

**D8904 (flags; notes only). unnamed_net stays /O2.** Retest with the match30 sources: /O2 23, /Ox /Ow 23,
/Ox /Ob0 /Ow 23, /O2 /Ow 23 of 31; /O2 matches nw_buffer_push 0x517780 (the binary reloads first_seq after
the stores, which /Ow does not), the /Ow sets nw_accept_reliable 0x518ce0. Plain `slot->addr = *from;`
does not make 0x517780 match under /Ow. No change to vc6-flags.txt.

**D8800 (match29; 13 units, cl 12.00.8966). Source shapes that make 8966 emit Sum.exe's code** (each verified by a
byte match; details in the match29 NOTES.md; no header changed):
- D8302 extended: every multitrade handler reads its fields with `int a, b; memcpy(&a, data, 4); memcpy(&b, data + 4,
  4);` (mp_handle_it_rqst / deny / acpt / addi / rmvi / cncl / qury / stat, 8 functions); mp_handle_respawn,
  mp_handle_forcepr, mp_handle_adjust_hp (all four fields), mp_handle_status_cure (the status read after the lookup:
  `int h, status; memcpy(&h, ..); c = lookup(h); memcpy(&status, data + 4, 4);`), mp_handle_srv_req (`memcpy(&type,
  ..)` then per case `memcpy(&a, data + 4, 4); memcpy(&b, data + 8, 4);`). A packed struct is one 17-byte copy:
  `memcpy(&packed, data, 17)` (mp_handle_new_lep, the four floats and the sign byte).
- A float argument added with MP_ADD_FLOAT (its temporary is the binary's extra local): container_spawn_loot's lock.
- Locals read before the header for values the binary loads early: `ubyte state = (ubyte)mp_netgame_state; ubyte
  status = (ubyte)nw_network_status;` (mp_build_game_info), `int type = req->type;` before MP_BUILD_HEADER
  (ai_service_request); a byte the binary loads at the top of a loop body: `ubyte layer = (ubyte)p->layer;` first
  (mp_send_new_path).
- An int offset counter for a byte loop: `int offset = 4; c->skill[i] = data[offset++];` (mp_handle_character_skills).
- mp_handle_join_request: `int len` (not ushort) and the deny reason chosen by `reason = 0; if (checksum bad) reason =
  VERSION; if (reason == 0) { if .. reason = .. else if .. else { accept; return; } } mp_send_deny_join(reason, ..);`
  (the binary's deny block sits after the checksum test; gotos to one deny label at the end, or the deny label in the
  first block, do not give it).
- `sprintf(script, "%s_m1", level_script_table[level].name); ls = &level_script_table[level];` (mp_change_level: the
  binary indexes the table for the sprintf and keeps the pointer in EBP afterwards).
- Case bodies in the binary's jump-table target order: mp_handle_srv_req (0, 1, 2, 3, 6, 4/5, 7),
  ai_state::process_goals (.., HIDE, SNEAK, BACKSTAB, PICKUP_ITEM).
- `bool walk = false; if (goals[0].flags & 0x10) walk = true;` passed to goal_do_move_to / _timed (a byte slot; the
  binary tests with the jump table's bound 0x10 still in ECX).
- Condition shapes: `MP_IS_HOST() && c->type >= 4 && (c->type <= 5 || c->type == 9)` (character_is_locally_controlled:
  `jl false` straight away); `d->opening = 1 - d->opening;` for the 1-bit toggle (inc / neg / shl and the xor-and-xor
  insert; `!x`, `~x`, `^= 1`, `x == 0` give other code), `elapsed = 0;` before two_stage_set, `opening` stored before
  `moving` (door_start_move).
- Direct indexing instead of an element pointer: `goals[best].type = type; ..` (ai_state::goal_alloc: the priority store
  reuses the compare's address).
- Operand order of a vector sum: `d->closed_pos * e + d->open_pos * (1.0f - e)` (door_update: 8966 evaluates the right
  operand first, so the binary's (1 - e) product comes first).
- vsdk (/Ox /Ow): an unrotated scan loop `while (1) { result = top; if (top >= y) break; i++; top += line_h; if (i >= n)
  break; }` with `int n = p->num_lines` (ui_listbox::snap_scroll_top); in set_vscroll_fraction `fh =
  gr_get_font_height(..); hi = (n - 1) * fh + y;` and `if (v >= 0.0f) vscroll = (int)(v + 0.5f); else ..` (closer, still
  differs: the dead-slot copy of num_lines); ix_linesegment_sphere_chord_length with one `return result` at the end,
  the work nested under `if (!(-r > proj) && len + r > proj)` / `if (disc > 0.0f)` and `result = float_min(..); result
  -= lo;`; ix_sphere_polygon's edge pair as `edge_a = *verts[i]; if (i + 1 == n) edge_b = *verts[0]; else edge_b =
  *verts[i + 1];` in a plain for loop (instruction-identical; te still has its own slot, the binary uses r's).

**D8801 (game/multicontainer.cpp; match29). LOGIC FIX: mp_handle_item_drop reads the container lock as a float.**
container_spawn_loot sends `lock` (float, D1709) with MP_ADD_FLOAT; the handler read the second field as an int and
passed it to container_obj_create(const vector3 *, float), converting the bit pattern with fild (Sum.exe pushes the
dword unchanged, 0x4892da `mov ecx, [esp + 0x2c] / push ecx`). Now `float lock = ((float *)data)[1];`. Also the
orientation is `matrix orient; orient = matrix(rvec, uvec, fvec);` (the binary's nine dword copies from the
constructor's result). Still likely-unsure: store order of the three vectors, registers.

**D8802 (notes only; match29). Still different under 8966 in this pass's units** (forms tried in the match29 NOTES.md):
mp_handle_it_okay (the binary re-tests state 3 on the state-2 path: 9 forms incl. switch, local copies, inner switch);
mp_handle_status_add / mp_handle_new_path / mp_handle_obj_update / mp_handle_combat_sequence / seg2d_intersect /
ix_sphere_polygon / matrix3_unpack_orientation (locals in dead argument slots, D8203); mp_send_obj_update (frame order
of short / float locals: 60 random renames change nothing); mp_send_new_lep (MP_BUILD_HEADER's counter in ESI);
mp_handle_ai_disable (the forward's `from` in EAX; memcpy of the ushort gives the binary's `mov cx / and ecx, 0xffff`);
mp_handle_actv_gm (version kept in BL); ai_state::goal_find (the strength-reduced pointer's base field and the loop
exit layout); the ai_move EH functions (prologue pushes not shrink-wrapped in the binary); script_action_wander
(argument scheduling); ui_listbox::draw / process / wrap_text (register allocation), get_vscroll_fraction (the fstp
scheduled after two loads).

**D9000 (match31; second pass over 33 units, cl 12.00.8966, flags unchanged).** 845 -> 867 of 1,014 compared
functions, none lost (address by address, before8966.txt / after8966.txt in the pass folder). No header and no
vc6-flags.txt change. Source shapes D9001-D9006; what still differs D9007; markers D9008.

**D9001 (boss units). Source shapes that give the original code under 8966:**
- A vector built by an operator into an existing local: `vector3 front; front = c->pos + c->orient.fvec; ...;
  front = c->pos - c->orient.fvec;` reuses one frame slot for both spawn points (serpent_spawn_rider: frame
  0x18, and with one local `pos + fvec` gives the binary's fvec-first loads); `pos = owner->pos +
  owner->orient.fvec;` in the three minion loops (ghost_rider_boss_load: x / y through the temporary).
- The offense-point tag lookup through locals, as machival (D7700): `const char *name =
  offense_point_name(i); ca_character *ch = owner->instance->character; tag = ch->find_mesh_tag(name);`
  (ghost_rider_boss_load, phoenix_boss_load).
- Facing tests through the inline vector3::dot (`dir.dot(t->character->orient.fvec)`) on `vector3 dir =
  target->pos - boss->pos; dir.y = 0;` (serpent_evaluate_target). A hand-written three-term dot that 8966
  reorders: try all 48 orders (term order x operand order) with a script; machival_count_targets_in_reach
  matches with `dir.y * d.y + dir.x * d.x + d.z * dir.z` (8966 emits the terms in reverse), and `dist =
  list->dist;` read before the difference vector.
- Block-scoped loop locals let the temporaries share their slots: `float r; vector3 p;` inside the `if
  (distance < 4)` block (ghost_choose_move_target, frame 0x18), with `p.y = 0.0f; p *= r; p += boss->pos;`
  (the y store kept, the y sum not folded) and `ghost_summon_minion = slots[i];` before `ghost_cur_move =
  move;` (8966 emits the second store of a pair first, D8404).
- Shared `return 0` at the end of the function: `goto none;` from the stage 1 / 2 picks and `if (handle !=
  -1) set_random(..); else goto none;` in stage 3 (ghost_choose_attack: all three `je` to one block).
- Store order: `serpent_handle = owner->handle;` before `serpent_num_extra_parts = 0;`
  (serpent_boss_load).

**D9002 (vsdk units, D7502 flags).**
- An element-copy loop instead of memcpy behind a count test: 8966 makes the binary's `test / jle / lea esi /
  lea edi / rep movsd` of `for (i = 0; i < n; i++) dst[i] = src[i];` (vim_copy_bitmaps, vim_save_bitmaps).
- Counter updates before the list unlink: `skeleton_count--; skeleton_mem_used -= ..; list_remove(this);`
  (~skeleton: the binary stores the globals before the NULL links).
- A loop-invariant pointer read inside the loop is hoisted after the loop guard, a local before the loop is
  loaded before it: `((vfx_node_entry *)b->data)[i].hash` (effect_get_base); direct indexing
  `effect_bases[effect_base_record_index[i]].resident` instead of a pointer local (effect_base_release: the
  dead `lea` goes away); `unsigned int i` for the slot loops (rfx_preload's `jb`, closer).
- Two calls instead of a joined FILE pointer (packfile_fread: `if (CD) return fread(.., cd_file); if (!hdd)
  return 0; return fread(.., hdd_file);`, 8966 cross-jumps them with the binary's registers); the offset
  sum as a statement before each call (`offset += entry->sector * 2048; return fseek(..) == 0;`,
  packfile_fseek); the start sector in a local (`int sector = first_sector;`, packfile_assign_offsets).
- vfx_load: the result variable initialised to NULL and nested ifs (the binary's shared `pop` tail for the
  missing base); closer.

**D9003 (game screen units).**
- Element pointers with the def pointer declared first (`const ui_button_def *d = &defs[i]; ui_button_state
  *b = &buttons[i];`): skills_handle_click (with the key cases in the binary's body order ESC/S, C, I, R),
  skills_load_bitmaps (and the icon buffer at function scope, both buffers 128 bytes: frame 0x10c).
  journal_handle_click closer with the same pointers.
- Direct global-array indexing instead of a cached `int skill` (skills_draw_list: the list walked by a
  pointer the compiler makes, the skill re-read for each use), and the colour switch with `case 0:` as its
  own body beside `default:` (D7901: jump table 0 -> default, 1 / 2 -> highlight, 3 -> disabled).
- `if (skill != SKILL_BACKSTAB) { .. } ` with the backstab block last and `== 8 ? true : false` for the
  `sete` without `xor` (skill_is_available).
- character_use_skill: the hide / sneak payment test as `if (hide) { if (mode) goto paid; } else if (sneak &&
  hidden) goto paid;` (the binary's jumps; closer).

**D9004 (game/s3d.cpp).** level_free_s3d: `void *palette` at function scope and `void *data` in the light
loop block put palette in the frame's own dword and data in the dead `level` argument slot (the binary's
`push ecx` frame). s3d_load_lightmap: `short w, h` (the binary keeps the first read_int16 result as a short in
the dead argument slot and sign-extends at each use).

**D9005 (vsdk/unnamed_gr_line_0x53e520.cpp).** gr_clip_and_project_poly: the projection loop through a
`gr_vertex *v = list[i]` element pointer, the store `verts[i] = list[i]` re-reading the list (the binary's
EBP / spilled offset). Now differs only in the order of the two flag-byte initialisations (`xor al, al` before
`or bl, -1`; every declaration / statement order tried gives the opposite).

**D9006 (notes only).** Forms that came closer without a match (kept): phoenix_reposition_update (`vector3 b
= down[1]->pos; vector3 mid; mid = (a + b) * 0.5f;`), murod_start_leap (`dir = vector3(frand() - 0.5f, 0,
frand() - 0.5f); dir *= 15.0f; dir += t->pos;`, the arguments evaluated right to left as the binary does),
ghost_find_floor_below (`vector3 start(..); ray.start = start;` and `+ start.y`: frame 0x94 now right).

**D9007 (notes only; match31). Still different under 8966** (forms tried in the match31 NOTES.md):
anim_remap_in of serpent / ghost / phoenix (ready in the dead `c` slot, D7702); serpent_illusion_update (rem
kept in memory, the alpha divided by -1000 without 8966's negation fold); serpent_illusion_start (register
choice after the divide); effect_base_read (push esi before the bounds test), effects_parse_arr_line (8966
merges the two `return -1` blocks for every form tried), rfx_preload (`lea edi, [ecx + edx]` operand order);
packfile_read_directory (p in the `pf` slot); dialogue_box::draw (first loaded into EBP before i);
conversation_controller::camera_capture (the binary calls the vector3 constructor out of line but expands
the transform; an inline_depth probe matches the frame but not the transform); itemtrade / status list
draws and mp_charselect_page_down (register allocation of rows / x); matrix43::transpose_in_place (the dead
temporary stores); seg2d_point_left_of (dx / dy kept on the FPU stack); localize_unescape (the empty-string
test as `cmp byte ptr [ebp], 0` without the CSE of the first character); wav_open (nBlockAlign's operand
load order).

**D9008 (notes only). Markers.** 22 functions raised to "exact (VC6 byte match, match31; was ...)". Eleven
markers that said exact and differ under 8966 lowered to "likely (marked exact earlier; differs under cl
12.00.8966, match31)": 0x4d8940, 0x406bb0, 0x409cd0, 0x419530, 0x419bb0, 0x41b570, 0x42c0e0, 0x4b7d30,
0x4a4f60, 0x511920, 0x545340.

**D9100 (tools/vc6-compare.mjs; 2026-10-17). Relocation targets are checked; `--strict` / VC6_STRICT=1 fails a
function whose bytes match but whose call or data reference resolves to the wrong place.** Before this, relocated
fields were masked and only same-file calls and sum.symbols names were checked, as a warning. Now every relocated
field is resolved: a function of the file with an address comment; any function marker or global defined with an
address comment on its own line anywhere in the tree (functions and globals kept apart; `T a, b; // 0xA, 0xB`
read in order); sum.symbols; a label of the same function (jump tables); an absolute symbol; or, for constants of
the object (string literals up to their NUL, float constants, initialised tables with their own relocations
masked), the bytes at the original's target. Accepted as right: another overload of the same name (sum.symbols
name plus a suffix), a byte-identical function the linker folded (/OPT:ICF, when the callee is compiled in the
object), a `jmp expected` thunk. About 96% of relocated fields resolve; the rest (function-local statics, inline
helpers without markers) stay unchecked and are counted.
First strict sweep: 4,956 byte matches -> 4,931 strict. The 25, all fixed here:
- string literals that differ from the binary (45 copies): the console usage text ends "toggles it\n", not
  "toggles it.\n"; characterinfo_parse_name's message names "CharacterInfo.cpp." and ends in a newline;
  new_icam_load / new_icam_get's messages end in a newline;
- party_sort_by_species puts the party in the order Joseph, Flece, Rosalind, Jekhar (jump table 0x434338; the
  decomp had Joseph, Jekhar, Flece, Rosalind): a behaviour difference;
- float constants one unit in the last place off: 0.0025000002f (0.05 squared), 0.17453294f, GR_DEG_TO_RAD
  0.017453292f and GR_RAD_TO_DEG 57.295776f (pi / 180 and 180 / pi as floats);
- operand order where the bytes matched with the operands swapped (same value): the automap and world map label
  margins (left + right, top + bottom), automap_label_box_hover, gr_project_size, the skills / spells / status
  row positions, help_draw's footer y, dialogue_load_bitmaps' text box width, phoenix_pick_weighted_move's threat
  sum, gr_d3d_set_viewport, sound_emitters_init's list head, the play menu's button order (3, 4, 2), the five
  scrollbar drags (`thumb + (mouse - track - start)`), and key_msg_handler's two focus cases (KILLFOCUS first).

**D9200 (2026-10-18, levelrender_build_draw_lists 0x482e20: an inverted distance test found by the hybrid game).**
A culled monster or NPC (type 4 or 5) gets AI flag 0x40 when it is **farther** than 1200 (squared, about 34.6 m)
from the current character: `fcomp [0x581aa0]; test ah, 0x41; jne` skips the `or al, 0x40` when the distance is
smaller or equal. The decomp had the test the other way round. It compiles to the same size and still differs (the
frame set-up), so the byte sweep could not see it; a quicksave made on the hybrid game right after loading
differed from the original's in one NPC's record (position, heading, a timer -1 vs 100) in every run, and the
save-state search (`[Hybrid] Only`, half the functions at a time) named this function.

**D9201 (2026-10-18, Character::init 0x44e1c0: an inverted animation test found by the hybrid game).** Bit 0 of
`+0x320` (`CF_COMBAT_READY`) is set, and the prop tag cleared, when the stand and ready locomotion animations are
the **same** (or in multiplayer): `cmp ecx, [eax+0x5b8]; je 0x44e800` jumps into the `or ecx, 1` block. The decomp
(decomp37) read it as "different". After a level change with the full party every monster in Iona's exterior had
the bit set in the original's quicksave and clear in the hybrid's; the search over `[Hybrid] Only` named this
function, which differs in its frame size too, so the byte sweep could not see the error.

**D9202 (2026-10-18, conversation::parse_table 0x4fee50: the farewell test found by the hybrid game).** A speaker
gets a `"Farewell."` topic only when none of its topics has the `+Farewell` bit (bit 2 of the topic flags:
`shr ecx, 2; test cl, 1` at `0x50066d`). The decomp tested bit 0, which the table never sets, so every speaker
with its own farewell topic got a second one. After a level change to the Jade Temple and the sewer boss level,
the conversation state in the hybrid's quicksave had one more topic for those speakers; the search over
`[Hybrid] Only` named this function (`unsure`, 6,348 against 6,976 bytes, so the byte sweep could not see it).

**D9203 (2026-10-18, blood_splot_place 0x468760: x87 precision, found by dual calls).** Each size case is
`fld radius; fmul k; fst [radius]` and then multiplies `u` by the value still on the FPU stack (80-bit, not
rounded), while `v` uses the stored float (`0x4688df`..`0x46890a`). Written as `radius *= k;` then `u = u *
radius;` the product used the rounded radius, so a decal's half-axis was one bit off in about 4 of 10 calls
(`[Hybrid] Dual`, compared against the original on the game's own calls). `u = u * (radius *= k);` keeps the
assignment's value on the stack: 8 of 8 calls the same. The function still differs from the original in its frame
size; the file keeps its 47 matches.

**D9204 (2026-10-18, party_spread_attackers 0x4b6c10: the function-static timer's address, found by dual calls).**
`static spread_timer timer` is the GameTimer at `0x247e91c` with its construction guard byte at `0x247e95c`
(`mov al, [0x247e95c]` at `0x4b6c1c`, `mov ecx, 0x247e91c` four times). Without the address comment the hybrid
game kept a second timer in the DLL, so the decompiled function and the original each had their own 2-second
pacing; the dual-call check showed the original's timer and guard changing and the DLL's not.

**D9300 (2026-10-18, blood_splot_place 0x468760: the remaining x87 rounding site).** After D9203 the small case
(size 0, `0x46899d`..`0x468a14`) still differed. The original stores the scaled radius (`fst dword ptr [esp+4]` at
`0x4689ac`), multiplies `u` by the 80-bit value left on the stack (`fmul` at `0x4689b2` / `0x4689b8` / `0x4689c4`),
calls the copy constructor of `u = ...`'s returned copy (`0x4689e7`, which empties the FPU stack) and only then
reloads the rounded float for `v` (`fld dword ptr [esp]` at `0x4689ec`, `fmul st, st(1)` at `0x4689f4` /
`0x4689fe` / `0x468a08`). With the D9203 source VC6 expanded that case's vector operators completely (no copy
constructor call), never stored the radius and multiplied `v` by the unrounded value (`fld st(0); fld st(0)` then
`fmul st(0), st(1)` / `st(2)` on `v`), so `v` was one bit off whenever rounding the radius to float changed the
product: the decal half-axis written by decal_place (`0x2480ab4` / `0x2480bdc`). The large and medium cases already
agreed in value (stored radius for `v` in both).

**D9301 (2026-10-18, blood_splot_place 0x468760: no byte match; source shape found, 29 variants).** Found in the
original and now in the source: (1) `vector3 v = u.cross(vector3(0.0f, 1.0f, 0.0f));` gives the original's frame
(`sub esp, 0xb8`, two more temporaries than the written-out `0 - u.z, 0, u.x - 0`) and the same expansion in the
small and medium cases (the copy constructor calls at `0x468933` / `0x468985` / `0x4689e7` / `0x468a3d`); (2) `off *=
radius; off.y = 0.0f;` (y cleared after the scaling) lets VC6 carry y = 0 into `pos += off`, which gives the
original's `mov eax, [pos.y]; mov [pos.y], eax` (`0x46879f` / `0x4687ab`); with y cleared first, `pos.y + off.y` is
computed. The code up to the switch then matches except `fld st(0); fsub u.z` where the original has `fld u.z; fsubr
st, st(1)` (`0x46881b`, same value). Still different: (a) with the cross product VC6 calls `vector3::operator*(float)`
out of line in the large case (the stored, rounded radius is passed), while the original expands it (`0x46885a`..
`0x468888`, the unrounded value as in D9203). The case order decides which case loses its expansions (2, 1, 0 and 1,
2, 0 orders tried: the first case written gets the most), and writing `pos += off` out per component frees enough
for the large case to expand, but then x is `pos.x + off.x` where the original has `off.x + pos.x` (`0x4687c0`), so
the original used `pos += off` and saves its inline budget elsewhere (not found: per-component `off *= radius`, a
named `up` vector, `int bm = -1`, `vector3 v(...)`, a len() written out). (b) The original loads the radius once for
`v = v * radius` (`fld [esp]`, then `fld v.z; fmul st, st(1)`, `0x468938` / `0x4689ec`; this load-once pattern occurs
nowhere else in Sum.exe with a stack local); VC6 here multiplies by the memory operand three times. Not changed by
`(float)radius`, a comma expression, `radius *= k;` as its own statement, a scoped or function-level `s` (VC6 gives
`s` the radius's slot, but still memory operands), `vector3(v.x * radius, ...)` or taking `&radius`. In value the
source is now the original's in the small and medium cases and differs in the large case (u with the rounded
radius); with the per-component `pos` addition it would agree in all three (814 against 810 bytes), not adopted
because of (a)'s operand order. The file keeps its 47 matches.

**D9400 (2026-10-18, camera::start_sequence 0x4792c0 and camera::update_sequence 0x479480: the fov constant, found
by dual calls).** Both store `fov` (`game_camera+0x18c`) as `fld dword ptr [seq + 0x08]; fmul dword ptr [0x581a58];
fstp dword ptr [esi + 0x18c]` (`0x479401`..`0x479414`, `0x4795f5`..`0x479603`), the same three instructions in
ours: no intermediate is kept or rounded differently. The one-bit difference was the constant: `0x581a58` holds
`0x3ea2f983` (0.31830987, the float nearest 1/pi), while the decomp's `0.3183099f` rounds to `0x3ea2f984`.
Written as `0.31830987f` (as in unnamed_lights, `* 0.31830987f`; `(1.0f / CAM_PI)` gives the same bytes). With the
right constant both functions are byte matches once their layout is: start_sequence computes the fov before
`flags |= CAMF_SEQUENCE` (the `fld`/`fmul` come before the flag's `mov`/`or`/`mov`), and update_sequence has no
local copy of the shot pointer (the original reloads `[esi + 0x13c]` for `sample`, `get_state` and the fov read and
saves `ebp` only inside the transform branch), so the member is cast at each use. The file goes from 39 to 41
matches (strict).

**D9500 (2026-10-18, closest_approach_time 0x4b1a00: x87 precision, matched).** The original keeps dv's three
components on the x87 stack, forms the squares z, y, x and stores each as a float in a dead slot (`fstp [esp]` z²,
`fstp [esp+8]` y² in va's argument slot, `fst [esp+0xc]` x² in vb's, hence `push ecx`; `0x4b1a19`..`0x4b1a2d`).
The test adds the unrounded x² still in st0 to the two stored floats (`fadd [esp+8]; fadd [esp]`, `0x4b1a31`), the
division forms the sum again from the three stored floats (`fld [esp+0xc]; fadd [esp+8]; fadd [esp]`,
`0x4b1a7c`). With the test and the division both on the named squares, VC6 treated the two sums as one common
subexpression, computed it once and kept the squares in registers, so the test's sum and the denominator were
both the 80-bit value (the denominator rounded once at `fst`). Matching source: the test is written on the
products, `if (dv.x * dv.x + dv.y * dv.y + dv.z * dv.z < 0.0001f)`, and the division on the named squares,
`/ (xx + yy + zz)`. The two sums are then different expressions; the squares (CSEs of the products) are stored
because the second sum reads the named floats. Strict match; the file goes from 8 to 9 matches. Not tried this
round: none of the earlier 23 order permutations. Also not matching: `dv.len_sq()` / `dv.dot(dv)` in both
places, a `float d = dv.len_sq()` local, products in both places, `dv.len_sq()` in the test with the named
squares in the division (stores x² instead of keeping it), a `vector3 sq` of the squares.

**D9510 (2026-10-18, ai_state::path_closest_point 0x4b2670: x87 precision at the path start point, fixed; no
byte match).** The rounding site is `q = path->pos - path->dir * 50.0f` (`0x4b275d`..`0x4b279f`): the original
stores dir.x*50 and dir.y*50 as floats and keeps dir.z*50 unrounded (`fmul [0x580de8]; fsubr [eax+8]`), so q.x
uses the rounded product and q.z the unrounded one. Written as `vector3 q; ... q = path->pos - path->dir * 50.0f;`
VC6 went through `operator=` (which returns a copy) and kept dir.x*50 unrounded and rounded dir.z*50 instead
(`fsubr [eax]`), so `prev` (from_xz of q) could differ by one unit in the last place in x or z, and with it the
closest point and `travel_dest.x` / `.z` (`[edi+0x54]` / `[edi+0x5c]`; the hybrid game's differing floats
0x245F3A4 / 0x245F3AC are 8 apart, as these two fields are). `vector3 q = path->pos - path->dir * 50.0f;`
(initialised in its declaration) gives the original's x87 sequence instruction for instruction. Setting
`seg.b = &cur` inside the loop after `cur.from_xz` gives the original's store (`mov [esp+0x64], ecx` in the loop;
`seg.a` alone before it). The look-ahead point `p` already rounded the same way (T.z unrounded in both).

Still differing (about 32 variants, none a match): (1) the frame is 8 bytes larger in the original (`sub esp, 0x60`
against 0x58; original slots from the frame base: best_d -108, closest -104, best -96, look -88, cur -80, prev -72,
the fvec * ahead temporary -64 (the label's unwind slot, `lea ecx, [ebp-0x40]` at 0x57c990), get_center's c -52,
p / q / the dir * 50 temporary -40, seg -28, 8 unexplained bytes at -20; ours puts seg before the temporaries
and has no gap); (2) p.z is `fld c.z; fadd st, st(1)` with T.z popped afterwards (`fstp st(0)` at 0x4b2747), ours
`fadd c.z` consuming T.z: same value, other bytes; (3) the prologue stores `found` / `n` before `push 1000000`
(declaring them before the profile_label gives that order, but no other function here puts anything before the
label, so it is not applied), and zeroes best.y, best.x, closest.y, closest.x in that order (ours closest first).
Tried for p without a match: a named t / c in either order and either operand order, `p += ...`, one expression
with the divide inline, `fvec * ahead + get_center()` (evaluates c first), a temporary argument to from_xz; for
the frame: seg declared after the block, no block, `vector2 diff = look - closest`. The file keeps its 9 matches.

**D9600 (2026-10-18, ai_choose_attack_slot 0x4b6a90: where the x87 precision differs).** The attacker's centre
is built by an out-of-line call of vector3(x, y, z) (`0x4b6ad8`..`0x4b6af9`: the three sums are rounded to float
when pushed as arguments, `call 0x43b580`), the target's centre is stored to a float local (`fstp [esp+0x34]`
.. `[esp+0x3c]`), and `d` is the difference of the two stored vectors (`0x4b6afe`..`0x4b6b2e`). VC6 expands the
constructor in every source form tried, keeps the x sums of both centres on the FPU stack and subtracts them
unrounded (`fsub st(0), st(1)`); y and z are rounded in both. So `d.x` (and after `normalize_safe` the whole
direction) differs in the last bits, and from there the cross product and the asin. The tail is the same in both:
`fabs; fst [s]; call _CIasin` on the 80-bit value, `fmul 0.7957747f`, `fimul sign`, `fiadd best`, the result
returned in st0 without a float store (VC6 puts the exact `fimul` by +-1 before the `fmul`; the value is the same).
The dot products: the binary sums `sd.z*d.z + sd.y*d.y` first, then `sd.x*d.x` (`0x4b6b50`); the source's
`sd.z * d.z + sd.y * d.y + sd.x * d.x` compiled as x + y + z. `sd.dot(d)` (vector.h: VC6 sums the z term first,
D504) gives the binary's order; applied. The frame (0x44 against 0x2c) is the two vector temporaries of the
constructor call and of operator-.

**D9601 (notes only). ai_choose_attack_slot is still different (28 forms tried).** No form makes VC6 SP5 call
the constructor out of line here: `(a->pos + a->centre_offset) - tc`, the same without `tc`, `get_center()` on
both, `vector3(sums) - tc` (temporary or named), operator= instead of the copy, a local inline helper returning
`vector3(...)` or `pos + centre_offset`; `#pragma inline_depth(1)`, `(2)`, `(3)` and `auto_inline(off)` change
nothing, `inline_depth(0)` calls every inline (and costs the file a match). Same symptom as
conversation_controller::camera_capture (D9007). Unless that is found, the decompiled function's result differs
from the original's in about the 7th digit (dual calls), from `d.x` only. Not forced with casts or float
temporaries.

**D9610 (2026-10-18, ai_state::approach_attack_slot 0x4bd2a0: vector expressions decide the rounding).** The slot
points are `dir * reach + dest` (`0x4bd488`..`0x4bd4e5`): `dir.y * reach` and `dir.z * reach` are stored as floats
(`fstp [esp+0x20]`, `[esp+0x24]`, operator*'s temporary) before `dest` is added, `dir.x * reach + dest.x` stays on
the FPU stack; the sums go through a temporary into `slot_pos[n]`. The mid waypoint (`0x4bd60a`..`0x4bd673`) is
filled in place: `attack_slot_direction(&mid, ..)`, `mid *= reach` (y and z stored, x kept), then
`mid += target->pos + target->centre_offset` with the centre's y and z in a float temporary (`fstp [esp+0x4c]`,
`[esp+0x50]`) and x added from the stack (`fadd st, st(1)`). The source wrote each component as
`dir.y * reach + dest.y` (all 80-bit), so slot y / z (the path destination: the floats around 0xCC497C) and mid
y / z were one unit off in the low byte now and then. Now `slot_pos[n] = dir * reach + dest;`,
`mid *= reach; mid += target->pos + target->centre_offset;` and `dest = target->pos + target->centre_offset;`
(the first 119 instructions are then the binary's except for register choice and the prologue's order; `lea ebx, [esi+0x10]` is &pos), plus
`int h = target->ai.attackers[i].handle;` (the binary keeps the handle in a local, `[esp+0x58]`). The FPU
sequences of all three points are now the binary's instruction for instruction; the values are the same.

**D9611 (notes only). approach_attack_slot is still different (16 forms tried).** Left: register allocation (the
binary keeps `this` in EDI and reloads `target` from its argument slot in the loops, VC6 keeps it in EBX), the
loops' pointer induction (`slot_ids` / `slot_pos` pointers in EBP / EBX, `found` and `direct` as stack bytes) and
the frame (0xb8 against 0xc8: the binary shares the vector temporaries with `mid`). `mid = dir * reach;
slot_pos[n] = mid + dest;`, `dest + dir * reach`, `mid = mid * reach + (..)` and `mid = dir * reach; mid += ..`
were tried and are further off or the same. The file keeps its 19 matches.

**D9700 (2026-10-18, avoid_area_add 0x4a9b40: x87 precision, the rounding site).** The jittered centre and
first axis are `0.3f * r + c` and `0.1f * r + a` through the vector2 operators: the product's y is stored to a
float temporary and reloaded (`fstp [esp+0x38]` at `0x4a9bb2`, `fld [esp+0x38]; fadd [esi+0xc]` at
`0x4a9bb9`; for the axis `0x4a9c04` / `0x4a9c0b`), while its x stays on the FPU stack (80-bit) and gets `d->cx` /
`d->ax` added directly (`fadd [esi+0x8]` at `0x4a9bb6`, `fadd [esi+0x10]` at `0x4a9c08`). The decomp's
component-wise `centre.y = r.y * 0.3f + d->cz` kept y unrounded too, so centre.y and axis.y (and through the
normalised axis the half axes written to avoid_obstacles) were one bit off in some calls (`[Hybrid] Dual`).
The rest of the first obstacle follows the same shape: `perp = vector2(-axis.y, axis.x)` with its y stored
(`0x4a9c37`), `ea * axis` and `eb * perp` (eb copied into the dead argument slot `[esp+0x54]`, the inline's
float parameter).

**D9701 (2026-10-18, avoid_area_add 0x4a9b40: the matching source shape).** Strict byte match (file 44 -> 45
matches; full strict sweep 4,956 -> 4,957, none lost) with: `centre = 0.3f * r + *(vector2 *)&d->cx;`,
`axis = 0.1f * r + *(vector2 *)&d->ax;`, a named `vector2 perp(-axis.y, axis.x);` before
`ha = d->extent_a * axis; hb = d->extent_b * perp;`, the size class 2 copy as
`(d->_unk28 + d->extent_a) * *(vector2 *)&d->ax` (likewise b), and r filled through a file-local float-argument
inline `avoid_set_xz(&r, rv.x, rv.z)` (fld / fstp copies; a function-scope r at `[esp+0xc]`, the frame's lowest
slot; block-scoped `vector2 r(rv.x, rv.z)` shared its slot with the temporaries and gave a 0x3c frame, and
`r = vector2(rv.x, rv.z)` copies through EAX). The casts stand for vector2 members of avoid_area_desc (D8601:
the original probably had them; the header keeps float pairs because spells.cpp and the fx unit fill them).
The last difference, `mov edx, [esi+0x2c]` (the owner argument) scheduled before `fstp [esp+0x1c]` (hb.x) at
`0x4a9c75`, went away only with `vector2::operator+` written `{ return vector2(x + v.x, y + v.y); }` (vsdk/math/vector.h):
the COMDAT 0x4aa3c0 computes y first and stores both after, which is that form (arguments right to left);
the old `vector2 r; r.x = ...; r.y = ...; return r;` form left everything else identical. Tried without
success (about 30 variants): an owner local, `const avoid_area_desc *`, `if ((ob = ...) >= 0)`, a vector2
`operator*(float)` member, ea / eb locals, perp as a temporary, perp in a block, declaration orders.

**D9800 (2026-10-18, script_editor_create_ui 0x4e0690: a loop bound found by reviewing never-run functions).** The
original fills the editor's size table for size classes 0-2 only: `mov eax, 0x255e9ac` (&char_sizes[0][1]),
`add eax, 0xc` per pass and `cmp eax, 0x255e9d0; jl` (0x4e06c0..0x4e0710), so char_sizes[3] (0x255e9cc) and
char_size_radii[3] (0x255a7d0) stay zero. The decomp looped `i < 4`, giving size class 3 ("huge") a pick box and a
radius of about 300. No shipped creature is huge, so the retail game shows nothing; a modded one would be picked
from anywhere in the editor. Now `i < 3`. The function differed before and still does (layout).

**D9801 (2026-10-18, options_drag_sliders 0x41fff0: the gamma constants, x87 precision).** The original multiplies
the gamma step by 0.1f and then by 1.5f (`fild; fmul [0x580214]; fmul [0x580210]; fadd [0x58020c]`,
0x420261..0x420271). Written as one expression, `n * 0.1f * 1.5f + 0.7f`, VC6 folds the two constants into one
`fmul 0.15f`, which differs in the last bit for steps 1 and 8 at 53/64-bit FPU precision. Split into
`v = n * 0.1f; v = v * 1.5f + 0.7f;` VC6 emits the original's sequence (v stays on the FPU stack). The function
still differs elsewhere.

**D9802 (2026-10-18, irc_channel::users +0x22: unsigned).** irc_get_channel_list reads the count with
`mov ax, [ecx+0x22]` into an eax already zero (0x4ae25e), which VC6 does for an unsigned 16-bit field; `short`
gave `movsx`. With `unsigned short users` (and irc_add_channel's parameter) irc_get_channel_list 0x4ae120 byte-matches.
Only a channel of 32,768 users or more printed differently.

**D9803 (2026-10-18, avoid_poly_add_search_nodes 0x4aa400: the hit point, x87 precision found by dual calls).** In
the Phoenix fight two of ten calls returned a length one bit off in the 7th digit. The original computes the
crossing as `(b - a) * t + a` through vector2 temporaries: `b.y - a.y` and its product with t are stored as floats
(`fstp [esp+0x2c]`, `[esp+0x34]`), x stays on the FPU stack (0x4aa517..0x4aa558). The decomp's per-component
`dx * t + a->x` / `dy * t + a->y` kept both unrounded. `hits[nhits] = t * (*b - *a) + *a;` compiles to the
original's sequence. The function still differs (frame 0x44 against 0x40); file 45 matches before and after.

**D9804 (2026-10-19, projector_record::to_lobj_space 0x4c0740: x87 precision found by dual calls).** In Jade Temple
all 10 checked calls differed in the low byte of each local plane's d (this +0x8c, +0x9c, ...). The original builds
`pt = n * d` and `pt += lo->pos` through the vector3 operators: x and y are stored as floats after the product and
after the sum, z stays on the FPU stack (0x4c07a0..0x4c07d8), and the dot product adds x and y, then z. The decomp's
per-component constructor kept the x product unrounded and the sums at 80 bits. `vector3 pt = n * d; pt += lo->pos;`
with the dot product written out gives the original's roundings (vector3::dot summed z first). The function still
differs (frame 0x34 against 0x44); the file keeps its 9 matches.

**D9805 (2026-10-19, decal_build_clip_planes 0x4bbf10: x87 precision, found by dual calls).** Two of ten calls
differed by one unit in the last place in `d->radius` (+0x14) and the projector's copy of it (proj +4, decal +0x3c;
`projector_record::set` 0x4c06c0). The original rounds `u + v` and `+ depth` to float through out-of-line
`vector3(float, float, float)` calls (0x4bc0b1..0x4bc0fe), then reloads the floats and sums (z^2 + y^2) + x^2
before `fsqrt; fstp [ebp+0x14]` (0x4bc103..0x4bc122). The decomp's per-component `diag` / `corner` locals let VC6
keep corner at 80 bits. `vector3 corner = *u + *v + depth; d->radius = corner.len();` reproduces the rounding;
`d->normal = u->cross(*v)`, `depth = d->normal * (...)` and planes built from temporaries
(`plane(&(-1.0f * un), &(d->pos - *u))`, 0x4bc184..0x4bc1a6) give the original's constructor calls. Not a byte
match (frame 0x58 against 0x50): VC6 expands the first `plane::plane` and `u->len()`, which the original calls
(0x4bc157, 0x4bc166); the expanded len() sums (z^2 + y^2) + x^2 where 0x4bc290 sums (x^2 + y^2) + z^2, the one
place the values could still differ. File: 9 matches before and after. (Agent work, merged and checked.)

**D9806 (2026-10-19, decals_draw_for_lobj 0x4bc6b0: the exception frame is a profile_label).** Four of ten dual
calls differed in `gr_poly_vertices[].v` (0x2ee6f38 +0): those are pointers, into `gr_poly_buffer_flush`'s stack
array `verts[25]` (0x545d70), shifted because the decompiled function had no SEH record (12 bytes) and so a
shallower frame; the x87 code (0x4bc9a7..0x4bc9dd, 0x4bca23..0x4bca8a) was already the original's. The prologue
0x4bc6b0..0x4bc6c5 is an EH frame, handler 0x57cbd8 -> FuncInfo 0x584580, whose one unwind action (state 0, set at
0x4bc6e4) is funclet 0x57cbd0 calling `~profile_label` 0x434650: a `profile_label label("decals_draw_for_lobj")`
as in the shadow unit. Also: the mode argument `gr::Mode(1, 1, 3, 3, 1, 0)` (the constant 0x118c21 at 0x4bcb1b
instead of the out-of-line Mode(int) constructor), `(bool)d->proj.inside` (0x4bc80e..0x4bc814), `1.0f < fade`. Same
length (1268 bytes), identical up to +0x25b; two scheduling differences left (the tri[2].z store at 0x4bc90b, the
operands of one commutative fmul at 0x4bca40). File: 9 matches. (Agent work, merged and checked.)

**D9807 (2026-10-19, conversation_controller::camera_blend 0x4d9ad0: x87 precision, found by dual calls).** Three of
ten calls differed in the last bits of the blended direction. `1 - t` is one shared subexpression, not a named
local: unrounded for the three axis_b products (`fst; fld st(0); fmul ...; fmul st, st(1); fmul`,
0x4d9c29..0x4d9c4f), read rounded for the angle (0x4d9ca4) and the position (0x4d9d30). The axis sum's
`vector3(float, float, float)` is called out of line (0x43b580, arguments pushed z, y, x, 0x4d9c53..0x4d9c9f), so z
adds the two unrounded products and x and y the rounded temporaries; 8966 expands that constructor and sums x
first, so the source writes the sum out z first with named products. The direction is
`matrix43(m).transform((0, 0, 1))`, folded to m[3] + m[2] off the constructor's result (0x4d9cdc..0x4d9cff, frame
0xd8); the angle compare is `fabsf` (`fcomp dword [0x57f028]`, 0x4d9bf7). Not a byte match: the binary calls the
copy constructor out of line for the flip's `operator=` result (0x4d9bde), which 8966 expands (the inlining budget
of D8708). File: 19 matches before and after. (Agent work, merged and checked.)

**D9808 (2026-10-19, script_editor_ray_hit_object 0x4e2f30: the exception frame is a profile_label).** Found by
listing every compared function whose original starts with the SEH prologue (`push -1; push handler`) and whose
VC6 output does not (after D9806): the handler 0x57ce38 -> FuncInfo 0x584870 has one unwind action, funclet
0x57ce30 calling `~profile_label` 0x434650 on `[ebp-0x20]`, the D9806 pattern. With
`profile_label label("script_editor_ray_hit_object")` the function is identical up to +0x4e (it differed from
+0x0); the file keeps its 92 matches. The only other function with a missing frame is the multiplayer
`mp_levelselect_draw_levels` 0x41d1d0 (left out with multiplayer). No compared function has a frame the
original lacks.

**D9809 (2026-10-19, blood_splot_place 0x468760: the large case's precision, closes D9301 in value).** Dual calls
still showed one-bit differences in the decal's half-axes (1 to 3 of 5 calls). D9301's cause (a): VC6 called
`vector3::operator*(float)` out of line in the large case with the stored, rounded radius, where the original
expands it (0x46885a..0x468888: `fld [esp]; fmul [0x5819bc]; fst [radius]`, then z, y, x as `u.c * st` each
`fstp dword` into the arguments of the out-of-line `vector3(float, float, float)` 0x43b580). Writing `pos += off`
out per component, `pos.x = pos.x + off.x; pos.y = off.y + pos.y; pos.z = off.z + pos.z;` (VC6 reverses the
operands as written, giving the original's `fld off.x; fadd pos.x` and `fld pos.z; fadd off.z`, 0x4687c0..0x4687d4)
frees the inline budget: the large case (0x46885a..0x4688da) is then the original's apart from relocations.
Remaining, same values: 0 - u.z as `fld st(0); fsub` against `fld u.z; fsubr` (0x46881b), and v in the medium and
small cases multiplied by the radius from memory against one reload (0x4688df..0x46890a, 0x46899d..0x468a14).
814/814 bytes; combat.cpp keeps its 47 matches; in the Serpent fight 30/30 dual calls the same. (Agent work.)

**D9810 (2026-10-19, ai_choose_attack_slot 0x4b6a90: the original's values through `#pragma inline_depth(0)`;
closes D9600/D9601 in value).** The binary rounds both centres and their difference to float: the target per
component (0x4b6a9f..0x4b6ad4), the attacker's sums as the arguments of the out-of-line `vector3(float, float,
float)` (0x4b6ad8..0x4b6af9), d from both reloaded (0x4b6afe..0x4b6b2e). Under 8966 every source form (named float
temporaries, z-first sums, `d -= tc`, per-component stores) keeps the first sum of each centre on the FPU stack and
drops its store, so d.x was computed from unrounded values; component order cannot help because the original
rounds all six sums. `#pragma inline_depth(0)` around this one function (above its address comment; between the
comment and the definition the compare tool loses the function) makes get_center (as 0x4da830), operator+
(0x45b680), operator- (0x45b6b0) and dot (0x4b24f0) calls that round through memory as the binary does; dot sums
z, y, x as at 0x4b6b50. This is a compiler directive rather than the original's source form (the original's
inlining choices under its build are not reproduced by 8966 here, D8708); it is used because it gives the
original's values: in the Murod Oni fight 18/18 dual calls the same (0/5 before). Not a byte match (301/381 bytes,
frame 0x38 against 0x44); the file keeps its 20 matches. (Agent work.)

**D9811 (2026-10-19, the definitions a standalone link needs).** A link of all 259 units into an EXE
(tools/link-build.mjs) left 52 C++ symbols declared and defined nowhere; the hybrid resolved them to Sum.exe
addresses. Now defined (agent work, checked; full strict sweep 4,961, none lost):
- Folded functions (the original linker kept one copy of identical code; the decomp names the callers' targets
  apart): the 13 `*_mouse_in_panel(int, int)` (`return true`, folded into 0x4063a0), the 8 `*_leave()`, the three
  rider `*_script_hook`, `pf_debug_search_step` and `ai_state::~ai_state` (empty, folded into 0x4f13e0), and
  `serpent_` / `phoenix_` / `tiger_dismount` (boss_dismount's body, 0x49c610). Each is in its own unit with a
  `// folded (0xADDR, name)` comment and no address marker, so the compare tools and hybrid-build do not treat it as
  a second function at that address; `/OPT:ICF` folds them again in the standalone link.
- Data, with the original's values read from Sum.exe: species_table (0x580e60, 223 x 12 bytes, .rdata, now
  `const`) and character_size_heights (0x5818d8) in characterinfo.cpp; team_enemy_masks (0x581904) in
  living_entity.cpp; character_size_radii (0x59d7c8, .data) in unnamed_ai_think_0x4b5140.cpp; equip_page_slots
  (0x5819d4) in item_paging.cpp; path_models_levels (0x59aa54, 9999999) in s3d.cpp; mp_join_unk_68e1bc in
  characterinfo.cpp (.bss); the seven gr_info_*_time in levelrender.cpp (.bss); string_buffer (0x257ed80, 0x200
  bytes) and game_mouse_z (0x257ef80) in unnamed_dialogue_table_0x4feb50.cpp (the only unit between the
  neighbouring .bss ranges); vfx_mesh_face_uvs (0x2c98a90) in effect_mem.cpp. config_combat_flag is
  Summoner_options + 0xc (0x60acd4), now a macro on the field as D604 did for config_solo_confirm.
- GUIDs (library .rdata in Sum.exe): IID_IKsPropertySet, IID_IDirectSound3DBuffer, IID_IDirectSound3DListener
  (0x5832e0..0x583300) in unnamed_ds_0x55bf90.cpp, the two DSPROPSETID_EAX20 GUIDs (0x5833b0, 0x5833c0) in
  unnamed_eax_0x521e40.cpp.
- `help_page` was defined twice: new_help.cpp's is the help screen's page (0x5bc920), gameplay.cpp's the help
  state's (0x60a66c, sum.symbols loading_help_page), two variables; gameplay.cpp's is now `state_help_page`.

**D9812 (2026-10-19, build options for the standalone test exe; both off by default, so the default build is
the source of the shipped code and byte matching is unchanged).** `SHIPPED_NOCD` compiles the no-CD edit the shipped
Sum.exe carries (D2600) in summoner_cd.cpp: the drive tests still run and are ignored, and both root strings are
".\" (the patched bytes listed in the file comment). `REBUILT_OPENGL` selects the game's own, never-selected OpenGL
renderer (0x68) in a window (200), as SummonerFix patches the shipped exe (0x43da64, 0x43db45), and caps
GL_MAX_TEXTURE_SIZE at 2048 in gr_opengl_texture_init as SummonerFix's glfix.c does (a modern driver's 16384 makes
the max_size^2 * 4 scratch buffer 1 GB, "Failed to allocate 1073741824 bytes", memory.cpp line 223). The
standalone link also needs, outside the decomp (src/link): the three DirectX 7 GUIDs Sum.exe takes from the DX7
SDK's dxguid.lib (0x583360..0x583380), import-library stubs for binkw32.dll (decorated stdcall names) and EAX.DLL
(ordinal 5), and for now a no-op libjpeg stub (libjpeg 6b is linked statically in Sum.exe and not rewritten).

**D9813 (2026-10-19, the $vis_base set is one s3d_vis_set: found by the standalone link).** s3d.cpp declared the
three words at 0x22e43f8 as separate globals (`level_vis_base_name`, `_count`, `_lobjs`), while level_vis_parse /
level_vis_reset / the forced lists store `(s3d_vis_set *)&level_vis_base_name` as a visibility set and
levelrender_fade_occluders reads its count (+4) and level objects (+8) through it. In Sum.exe and in the hybrid
the three sit together at 0x22e43f8..0x22e4403; in the standalone exe the linker placed them apart, so the "set"
read another variable's bytes as its count and index pointer, and the first level render crashed
(levelrender_fade_occluders +0x58, a level object index of 0x12bef80). Now `s3d_vis_set level_vis_base` (name,
count, indices); the code bytes are the same (s3d.cpp 28 matches, the render-scene unit 51, before and after).
Lesson: globals that the code uses as one record must be one object in the source; the hybrid cannot see this
(its globals are Sum.exe's addresses).

**D9814 (2026-10-19, three data tables wrong in the source, found by tools/link-datacheck.mjs).**
pxochat_channel_users_right (0x590038) is {211, 0x108, 0x152} and pxochat_channel_count_x (0x590044) {223, 0x117,
0x165}: the source had the hex digits as decimal (108, 152, 117, 165). levelrender_fan4 (0x59c368) is {0, 1, 2,
2, 3, 0}, not {0, 1, 2, 0, 2, 3} (the same two triangles in another vertex order). The functions reading them
byte-match, so the hybrid used Sum.exe's own tables; only the standalone exe used the source's.

**D9815 (2026-10-19, fx_frame turns a camera effect around wrongly: the prologue stall).** fx_frame (0x48a230)
case 3, an effect at the camera (fx_create_at_camera, the cutscenes' "...camera.vfx" shot effects): the original
(0x48a6c6-0x48a7df) copies camera_view_pos and camera_view_orient, then negates rvec and sets uvec = -(old fvec)
and fvec = -(old uvec). The source negated all three rows, so the effect faced away from the camera, and the
prologue's narration panels ("Nar-PRE-INTRO-camera.vfx") were not seen: the new game's opening showed one still
sunset shot. fx_frame is one of the 47 functions that SummonerFix's fixes patch, so the hybrid kept it original
and this was its first run as decompiled code. Found with the agent: the shot, the cutscene clock and the fx
handles matched the original, and a hybrid with `[Fixes] Enabled=0` showed the stall; skipping fx_frame alone
(`[Hybrid] Skip=0x48a230`) restored it. Still non-matching (the unit's 47 matches unchanged).

**D9816 (2026-10-19, two errors in functions that first ran decompiled in the standalone exe).** A read-through
against the disassembly of the 47 functions that the hybrid kept original only because the bug-fix module
patches them (fx_frame's other cases included) found two, both checked in the listing:
- Character::get_hit_by_spell (0x45dea0), spell 0xd (level drain): the original tests the target's level
  (0x45fe40: `movsx edx, word [this+0x600]; cmp edx, 1; jle`); the source's `if (level > 1)` read the function's
  local spell level, which hides the member. Now `this->level`.
- living_combat::trigger_reflex_ability (0x467670), a beneficial armour-triggered spell in single player: the
  original passes the attacker's handle (`[ebp]` = owner, +0x80; 0x467b0f-0x467b17) as spell_cast_execute's
  target, the source the defender's. The two single-player item calls cross their targets in the binary (the
  offensive one's was already noted, D5506).
The other 45 and the rest of fx_frame read as equivalent. Both functions still differ (no match lost). Not
reproduced in play: both branches are rare.

**D9817 (2026-10-19, the Create Game numbers are centred by width and height; game bug K9 withdrawn).** A
read-through against the disassembly of the 49 non-matching functions that the hybrid keeps original even with
`[Fixes] Enabled=0` (display, widescreen and key-binding patch sites: gr_init, key_mark, os_wndproc, ui_process,
world_mouse_frame, the UI and dialogue drawing) found one error, in gamecreate_draw_player_count (0x40a4f0) and
gamecreate_draw_limits (0x40a570). The sprintf arguments stay on the stack until the final `add esp`, so after
gr_get_string_size and three pushes `[esp+0x30]` (0x40a541) is the height, for y, and `[esp+0x34]` (0x40a553) the
width, for x. The source (and game bug K9) had read both as the height. Now `x - w / 2`, `y - h / 2`; K9 is
withdrawn (docs/sum-exe/game-bugs.md, "Claims that did not hold"). Both functions still differ. The other 48 read
as equivalent, so all 71 non-matching functions that the hybrid never ran have now been read against the listing
(the 22 of D9816's review and these 49).

**D10000 (m-1019 worker 1: characterinfo, unnamed_ai_think_0x4b5140, unnamed_experience_0x489930,
unnamed_murod_0x4c5140, gr_direct3d, gr_font, plus partial forms in sound_emitter, jadetemple,
script_internal). Source shapes that make cl 8966 emit Sum.exe's code** (each a byte match, strict):
- A call whose result is an argument of another call goes into a local first (D6005, still true under 8966):
  murod_ai_leap (`move_info *m = find_move(..)` before play_move), summon_stash_for_cutscene (`float t =
  get_anim_length(..)` before process, the - 0.01f in the argument).
- `c ? a : b` as a call argument is an if / else of two calls: level_load_preload (charinfo_mark_anims_loaded),
  statscreen_load_anim (charinfo_find_move 0x9e / 0x9f); characterinfo_alloc_level_buffers' apply_hero_model
  gives matching bytes that way (still `target`: the four anim-buffer-size loads come in another order).
- Commutative operand order: gr_d3d_clear / gr_d3d_zbuffer_clear `clip_left + offset_x` (as set_viewport);
  retaliate_on_hit `da * (intelligence * 4.0f) >= dt`.
- Chained assignment stores the right-hand target first: gr_d3d_make_line_verts `v1->rhw = v0->rhw = 1.0f`.
- Walking pointers / element pointers: experience_parse_level (`xp_level_gain *g`, g++),
  gr_d3d_tmapper_multipass's vertex loop (`d3d_tlvertex *d = &dv[j]` and a float local for sw; that loop now
  matches, the function still differs later).
- No walking pointers where the binary indexes: gr_d3d_read_backbuffer copies row i from
  `lpSurface + (y + i) * lPitch + x * 2` to `buf + i * w * 2`.
- Locals: gr_font_get_char's loop bound `int n = f->num_kern_pairs` (D6005); follow_repath uses ONE vector3
  for the path destination test and the predicted position (the binary has one slot) and a float local for
  the scaled think time; statscreen_load_anim passes `int anim = m->anim` (no reload after the store);
  level_load_preload's buffers are name[0x20] / hash[0x20] (the binary's slots: hash at frame +0x28).
- summon_restore_after_cutscene: `if (stashed_summon) { ...; level_mempool.offset = ..; return; }` then the
  offset store again (two copies in the binary).
- 8966 already puts a local in a dead argument slot by itself (level_load_preload's mp in pool's slot).
Tried without a match (kept as notes in w1-NOTES.md): sound_emitter_delete (D8701 two returns gives the entry
pushes; the link stores still schedule differently), ls_jadetemple_action (no `t` local gives the binary's
registers; item_name / line slots stay swapped for every name, order and scope), living_entity_setup_head
(`tag` local for $head), script_action_combat_mode (switch gives the binary's range compares),
d3d_compute_pixel_format_shifts (rewritten with per-channel `for (x_shift = 0, m = mask; ...)`: frame now
0x1c as the binary; register choice differs).

**D10001 (include/engine/characterinfo/characterinfo.h, game/characterinfo.cpp). HEADER CHANGE:
statscreen_load_anim's second parameter is int, not bool.** The callee reads it as a dword twice (0x448f8d
`mov eax, [esp + 0x14] / test eax, eax`, 0x44901b `mov ecx, [esp + 0x14] / test ecx, ecx`; a bool is read
as a byte), and its only caller, statscreen_refresh_anim (0x412ef7), pushes a full EAX set by
`mov eax, 1` / `xor eax, eax`. The caller (game/unnamed_interface_0x411270.cpp) is unchanged and compiles
to identical code (56 / 69 before and after). statscreen_load_anim now matches. mingw: 259 .cpp, errors only
in unnamed_decals_0x4bbe80.cpp (9, the same in the untouched base copy); full strict sweep of the tree: 4976
match, 0 lost against vc6-matches.txt.

**D10100 (m1019 worker 2; 19 units). Source shapes that make cl 12.00.8966 emit the original code** (each verified by
a byte match, strict sweep, no match lost; notes in m1019 w2-NOTES.md):
- A ternary passed as a call argument is an if / else of two identical calls, which 8966 tail-merges into one call
  with the arguments pushed before the branch (itemtrade_draw_gold_input 0x41a7b0, with `min(0, size - w)` in a
  local: `setg` for `0 < x ? 0 : x`, not `setns`).
- Where a result local already exists, assign it and return it once: `if (d > 0.5f) pan = dot; return pan;`
  instead of a second return (snd_calc_pan 0x5211f0, sound_emitter_calc_pan 0x521270); 8966 then reuses the dead
  argument slot for `pan` as the binary does. A switch whose cases each `return state = X` is written with each
  case storing the state and `break`, and one `return effect_stream_state` after the switch (effect_stream_update
  0x4408a0).
- A call nested in another call's arguments, or an `i++` in them, is hoisted into its own statement: `int h =
  *(int *)data;` (mp_handle_swi_sneak 0x488ab0, as mp_handle_swi_hide), `int net_b = mp_handle_to_net(b);`
  (living_combat::send_net 0x463540), `sounds_parse_entry(&tf, i); i++;` (sounds_parse_table 0x521a60).
- `&table[f(x)].sub[i]` takes the outer element pointer first: `glide_tcache_entry *e = &...[frame]; sec =
  &e->sections[section];` (gr_glide_get_section_rect 0x549500).
- A value chosen by a test is an if / else assignment, not an initialisation overwritten by the if
  (glide_msg_handler 0x549f50); a search result is set to NULL just before its loop, not at its declaration
  (state6_enter 0x43fb60).
- A vector difference with one component cleared and the inline dot, instead of written-out components: `d =
  opponent->owner->pos - owner->pos; d.y = 0.0f; ... d.dot(owner->orient.fvec)` (living_combat::pick_attack_move
  0x463e20; 8966 builds the difference in a temporary and copies z through an integer register, as the binary).
- A loop index that is only compared is `unsigned`, and an array element passed to a call is read into a local
  first: `const char *name = feedback_event_names[event]; tf.Expect(name);` (music_feedback_parse_tables
  0x4691e0: the strength-reduced pointer compare becomes `jb`, the count is cleared before the load).
- glide_load_dll 0x5473b0 matches with an odd form: the first GetProcAddress reads the global glide_dll_handle and
  all the later ones a local `h` copied from it just before (the binary pushes EAX for the first call and keeps the
  handle in ESI afterwards). Same behaviour; a local for all calls makes 8966 save ESI before the NULL test, which
  the binary shrink-wraps after it. Kept because it is the only form found that gives the bytes.
- levelrender_wave_lobj 0x48de60: the function-static GameTimer is declared after the lobj_find_textured_record
  call (the binary runs the guard test and the constructor after the call). Closer, still differs (the loop
  counter lives in a frame slot, frame 0xb8 against 0xb0).

**D10101 (m1019 worker 2; notes only). Tried and not reproduced under 8966** (each with the forms listed; the
first-difference offset did not move unless stated):
- The volume / pan clamp of snd_set_volume, snd_set_volume_in_group, snd_set_pan and snd_set_group_volume: the
  binary jumps out of line to both clamp stores (`jne lo; ...; je hi; jmp done; lo: ...; jmp done; hi: ...`);
  26 forms (if / else if, ternaries, a macro, nested ifs, gotos, switch, separate ifs, `<` / `>`) all give the
  fall-through layout.
- snd_calc_volume_3d / sound_emitter_calc_volume: the binary keeps `zero` in a frame slot (stored 0 at entry,
  loaded when the divisor is 0); 8966 folds it to the constant for five forms (late assignment, a result
  variable, declaration order, if polarity, `return zero`).
- effect_stream_finish_entry 0x440930: the binary places the code after the switch directly after the RFX case
  (jump table: -1 -> the shared `xor eax, eax` of the error tail, 0 -> the effect_stream_next block, 1 RFX, 2 MVF,
  3 VIM; MVF / VIM jump back into it). default placements, case reordering, the tail duplicated per case, a
  `goto next` label after RFX, and fall-through `case -1` forms all differ.
- living_combat::chain_check_window: the binary does not merge the identical `return 100.0f` / `return 0.0f`
  blocks of the two window branches but shares the chain-learn tail; 8966 merges the returns. A `goto learn` form
  differs the same way.
- vector3::unrotated: all 48 term / operand orders of the z component (and structural forms: row references,
  a float array, locals, temporaries) give the same z, x, y order of the terms; the binary has z, y, x.
- vector3::length_quick: `t + t * 0.5f` is folded to `t * 1.5f` by 8966 for ten forms; the binary multiplies by
  0.5 and adds. vector3::random_unit_vector / random_direction / random_direction_in_cone: 8966 keeps the
  squared component on the FPU stack (`fmul st, st(1)`), the binary pops it (`fmulp`); eleven forms.
- itemtrade_count_party_rows (`inc dword ptr` on the global: four forms), mp_handle_swi_path (the walk flag
  through the dead argument slot: six forms), serpent_part_is_busy_or_asleep (EBX saved only around the flags
  test: five forms), movie_play (File at [esp] below the path: four declaration forms), effect_stream_start
  (the final state store before `mov eax, 1`: four forms), itemtrade_draw_list1 (rows in memory, x in EBP: four
  forms), get_protection (e6 kept in EDX and the bless tail placed after the no-armour branch), fire_weapon_missile
  (frame 0x174 against 0x180; if / else and local forms for the arrow spell), vector2_closest_point_on_segment and
  vector2_dist_sq_to_line (`fadd st, st(1)` leaving a product on the FPU stack: four forms).

**D10300 (game/new_pxochat.cpp). Dead compares are merged identical branches.** pxochat_checking_version_popup_frame,
pxochat_version_error_popup_frame and pxochat_new_version_popup_frame each have a compare without a branch of its
own (`cmp eax, ecx` with a zero register, `cmp eax, 1` before a call). Build 8966 leaves such a compare when two
identical blocks follow a test and it merges them: `if (choice < 0) { checked = 1; state = 0; return key; }`
before the same two stores, and `else if (choice == 1) gameseq_set_state(1); else gameseq_set_state(1);`. Same
behaviour; all three now exact.

**D10301 (game/unnamed_snow_0x4946a0.cpp). SNOW_FALL_RATE is a const float variable (0x581b0c).**
snow_cell::fill_count divides by it in memory (`fdiv dword ptr [0x581b0c]`); with a literal, VC6 replaces the
division by a multiplication with the reciprocal (every literal form tried: float, double, 8.0f / 3.0f, a local).
`const float snow_fall_rate = 2.6666667f;` gives the original code; snow_update still matches. fill_count exact.

**D10302 (game/unnamed_evil_urath_0x4c4170.cpp). A vector assigned after its declaration is copied through a
temporary.** evil_urath_ai_leap_land builds `m.fvec + target->pos` in a temporary and copies it into facing with
integer moves: `vector3 facing; facing = m.fvec + evil_urath_leap_target->pos;` (an initialisation, with the
constructor or with `=`, builds facing in place). Now exact. In evil_urath_ai_leap_travel and _leap_rise
`owner->pos += owner->orient.fvec * step`, `(target->pos - owner->pos).dot(owner->orient.fvec)` and the
`vector2 d(x, z)` constructor (arguments evaluated z first) give the original instructions; both still differ in
frame slots only (leap_travel's d and the second temporary share a slot in ours; leap_rise's top and dist are
swapped).

**D10303 (game/unnamed_evil_urath_0x4c4170.cpp). evil_urath_leap_check_path: start / end constructed in place,
the wall count in a local, the if-form bool return.** `vector2 start(boss->pos.x, boss->pos.z); vector2
end(target->pos.x, target->pos.z);` after the *dist store, `int n = cell->num_walls;` as the loop bound (the
binary loads the ushort once, `mov si, word ptr`), and `if (*dist < FLT_MAX) return true; return false;` (mov al).
Now exact. The game bug noted there (*dist holds the last crossing, not the nearest) is the binary's.

**D10304 (game/unnamed_giant_salamanka_0x4c8030.cpp). giant_salamanka_ai_frame copies the target through the
AI record:** `target = giant_salamanka_target_handle; owner->combat.target = target;` (one load, two stores of
EAX). Now exact.

**D10305 (game/unnamed_tiger_0x499e50.cpp). Branch shapes.** ai_state::tiger_choose_attack: the sub-mode test reads
`submode` in place (no int local), and the stage 3 cases end in `break` to the shared `return 3` (the binary's
case 0 falls into it). ai_state::tiger_stage_move_update: the move tests sit inside `if (emerge_move >= 0) { ... }`
with one `return false` after (the binary jumps there with `jl`), not behind an early return. Both exact.

**D10306 (game/new_hud.cpp). quicksave's frame: the list head is a whole file_find_node, filename[32],
message[64].** Re-reading the stack offsets with the pushes counted: buttons 0x0, list head 0x4-0x3b (0x38 bytes,
as new_charselect.cpp's), filename 0x3c, message 0x5c (frame 0x9c). The source had a bare {next, prev} head,
filename[28] and message[72]. Now exact (the message buffer is 64 bytes in the binary: a localized "Quick save
successful." / "Error during quick save." longer than 63 characters would overflow it, as in the original).

**D10307 (game/new_saveload.cpp). saveload_capture_thumbnail passes the fov through a float local**
(`float fov = camera_fov;`): the binary loads it with fld and stores the argument with fstp; passing the field
gives an integer push. Now exact.

**D10308 (game/unnamed_tiger_0x499e50.cpp). tiger_pick_wander_point: dir scaled and moved in place, the dot
product written x, z, y.** `dir *= frand_range(rmin, rmax); dir += target->pos;` (the binary has no separate p;
frame 0x18) and `dir.x * to.x + to.z * dir.z + to.y * dir.y` (found by trying all 48 orders of terms and factors:
build 8966 reorders the sum, and only this order gives the binary's z, y, x code). Now exact.

**D10309 (vc6-flags.txt). vsdk/unnamed_vif_0x5329f0.cpp is compiled /Ox /Ob0 /GX /MT /Ow.** The binary calls
vector3::operator= (0x413190), operator- (0x45b6b0), operator* (0x50f790) and operator+ (0x45b680) out of line in
vif_emitter_spawn_point; with /Ob0 the same 8 functions match and the 7 others come much closer (spawn_point's
frame is right, vif_emitter_update and vif_draw_particles differ late). D7502's rule holds (no match lost).

**D10310 (vsdk/unnamed_d3d_tcache_0x54bb30.cpp). gr_d3d_get_section_rect reads the section in place**
(`e->sections[section].x`, no section pointer): register choice of the original. Now exact.

**D10311 (notes only; worker 4 of 2026-10-19). Still differing, no source form found:** the max(x, 0)
`and` register (D7506) in assess_page_up, item_info_popup_page_up, saveload_page_up, pxochat_init and
pxochat_add_chat_line (its only difference: `and ecx, eax / mov [g], ecx` against `and eax, ecx / mov [g], eax`,
the short form); shrink-wrapped pushes of ESI / EDI (irc_add_user, vif_curve__evaluate); vif_emitter_spawn_point
and vif_curve__evaluate's segment search, where the binary sends the loop exit straight to the "last point"
block and tests only `i < 0` after a break (no condition order, `goto` or split if reproduced it);
door_pick_by_mesh's lock compare (`fld [edx+0x58] / fcompp` after the sum: every compare form tried gives
`fcomp [edx+0x58]`; with `open, locked` in the success block the frame is right, 0x108); mem_report_line's
`65 - len - strlen(text)` order; pxochat_page_down's result register; hud_drag_icon_begin (the binary keeps the
read of the old bit-field word); the zero register of tiger_ai_frame and pf_read_edges' `lea ebp, [eax]`.
The summoner_cd.cpp differences are the shipped no-CD patch (D2600).

**D10400 (game/items.cpp; match round 2026-10-19 worker 5). Source shapes for two item functions.**
item_create (0x44b2d0): the result is a local set to NULL after the range test (`it = NULL; switch (kind) { case
0: it = item_create_weapon(type); break; ... } return it;`): the zeroed EAX is both the switch base (`sub edx,
eax`) and the NULL of the other kinds. character_meets_item_requirements (0x44b550): the non-party test is an
early `if (c->type != 7 || (c->flags & 0x100)) return true;` before the species and skill checks (the nested
`if (c->type == 7 && ...) { ... }` gave the other register assignment for c and type). Both exact; behaviour
unchanged.

**D10401 (game/gametarget.cpp). container_examine reads its format strings into locals first.** The binary loads
XSTR 633 / 634 into EBX / ESI before calling localize_item_name (0x43c4f9, 0x43c51a); `char *fmt = XSTR(...);
sprintf(text, fmt, ..., localize_item_name(...));` in each branch gives it (D6205's "format string first").
Exact. Also object_try_unlock_or_open (0x43c060, still differs): the key search is an if / else of the inventory
test and the party loop with `Character *c = living_from_handle(p->handle)` before character_find_equipped_type
(the binary pops both calls' arguments at once), and the client's door branch tests `d->key` first;
world_pick_lobj (0x43a770, still differs) compares `best > h.t` (`fld best / fcomp t`). Same behaviour.

**D10402 (game/level_scripts.cpp). Float fields written to the save buffer through a parenthesised float
temporary.** write_character (0x4cf6c0) and automap_label_add (0x4d12b0) store the float as `fld / fstp [temp] /
mov reg, [temp] / mov [dst], reg`, the temporary in a dead argument slot (D6306 (4) is reproducible under
8966). `float t = (src); *(int *)(dst) = *(int *)&t;` gives exactly that (D8301's parenthesis effect; the
original probably used a save macro with a parenthesised argument). Both exact. The same form makes
write_item_objects (0x4d2370) the original's size and instruction sequence (with the container loop over an index,
the binary's `jl` against the biased end); it still differs in the frame order of count and the temporary.
automap_label_remove (0x4d1390): rx / rz read as integer copies (`*(int *)&rx = ...`, the binary's `mov / mov
[esp+x]`) and `off = 4` before the count test: same mnemonics and size, still differs in the slot of `start`
(declaration order and block scopes tried, no effect).

**D10403 (game/unnamed_ai_combat_0x4bce60.cpp). Vector temporaries in the approach functions.**
approach_random_offset (0x4bcea0): `vector3 *from = &owner->pos; dir = vector3(target->pos.x - from->x, 0.0f,
target->pos.z - from->z); dir.normalize(); dest = dest - dir * r;` and the path-found branch first (`if
(path_find_new(...)) { ...; return true; } return false;`). approach_target_point (0x4bcff0): each candidate is
formed in dest (`dest = bone_pos + target->centre_offset`), distance(&owner->pos, &dest) (symmetric), the best
copied to best_pos and back to dest after the loop (`_unk7c = best_index; dest = best_pos;`); `best = -1.0f`
assigned before best_pos (otherwise best takes the dead argument slot instead of the loop pointer). Both exact;
behaviour unchanged.

**D10404 (notes only). Still differing in this worker's files, by kind.** (1) Register choice with identical
code (fast_inv_sqrt / fast_sqrt / fast_sqrt_refined: EAX / ECX swapped; state_title_enter, loadgame_level_list_cb,
level_format_display_name, get_int, level_item_has). (2) Jump threading: 8966 here re-tests `i >= n` after a
search loop (get_speaker_bit, read_level_items, level_item_*), 8168-style threading is what our build gives
for every source form tried (pointer loops, `if (i < n) {...}`). (3) Tail duplication / no cross-jumping in the
binary (savegame_location_allowed's three `get_flag ? 2 : 0` cases, object_try_unlock_or_open's message tails).
(4) Stack-slot order with the same frame size (write_item_objects, automap_label_remove,
status_handle_click). (5) Prologue placement of a callee-saved push (consider_life_drain, int_to_string_with_commas
where a `char *src` pointer gives the binary's count-down loop but the `push ebp` sits before the first strncpy
in ours; plainstring.cpp left unchanged). has_conversation_data: the binary loads the data dword into EAX before `test`; every bool form tried
compiles to `cmp [eax], 0`.

**D10500 (w6 pass, cl 8966; vfile, multispell, netpackets, equip_packets). Source shapes that give build 8966's code
for byte-buffer reads and writes.** Each verified by a byte match:
- FileReader::read_int / read_int16 / read_uint16 / read_int8 / read_uint8: the value is read through `pos` itself
  before the position moves (`if (pos + 4 >= size) return 0; int v = *(int *)(buffer + pos); pos += 4; return v;`);
  the old `int p = pos` copy gave another register choice. read_float copies the bytes with `memcpy(&v, buffer +
  pos, 4)` (the binary's `mov [esp], eax / fld [esp]`); FileReader::read is `memcpy(buf, buffer + pos, len); pos
  += len;` (the store after the copy); read_vector / read_quaternion read one component and add 4 to pos for each.
- Packet handlers that read one int: `int v; memcpy(&v, data, 4);` (mp_handle_summon_allowed, mp_handle_summon_quick;
  8966 then reuses EAX as the binary does). mp_handle_equip_item reads slot (data[1]) before type (data[2]).
- Float packet fields go through a float temporary that stays in memory (`float put_ = v; memcpy(...)`): the
  request's extra point is a vector3 (mp_write_spell_target_point), and the per-mode targets are floats where the
  binary stores the temporary (ftarget[] in modes 1, 2, 3b, 4b, 5h, 6a, 7a, 7b, 10b of mp_write_spell_targets);
  mp_spell_cast_sync uses MP_ADD_FLOAT for the target point (frame 0x204) and `item = none; if (req->item) item =
  ...` for the item index.
- netpackets: the rate prefix of multi_process_packet is an if / else of two strcpy calls (not `strcpy(rate_name,
  reliable ? ... : ...)`); multi_send_packet and multi_send_packet_buffered index multi_*_send_bufs[id] directly
  (no `buf` pointer local).

**D10501 (game/unnamed_interface_0x411270.cpp). ui_draw_text_box keeps its corners as four vector2 points**,
declared top-left, top-right, bottom-left, bottom-right (`vector2 a(x, y), d(x + w, y), b(x, y + h), c(x + w, y +
h)`), the lines a-b, b-c, c-d, d-a: the binary's 8 float slots (frame 0x20) and line argument order; 1 of the 24
declaration orders matches. ui_message_box_hit_test computes y in one expression with the font-height call last
(`text_pos[r][1] + text_size[r][1] + y0 - gr_get_font_height(-1)`): resolution_index is read before the call.

**D10502 (w6 pass). Vector temporaries under 8966 (D6306 (1) no longer applies).** Where the binary builds a
vector3 temporary and copies it (frame 12 bytes larger than ours), `vector3 v; v = vector3(a.x + b.x, ...)` gives
it (evil_joseph_spawn_helpers; the component order inside the constructor does not matter). Where ours is 12
bytes larger, a block-scoped vector shares the slot of a dead one (claim_attack_slot's `s` in the loop body: frame
now 0x24 as the binary; the function still differs in the operand order of the first get_center). The hand-written
dot product of add_skill_from_behind is the inline `d.dot(back)` (the binary keeps -fvec on the FPU stack);
offset_point_sideways is `d = *pos - owner->pos; d.y = 0.0f;`.

**D10503 (include/engine/ai/boss_ai.h, game/unnamed_murod_oni_0x4c5c20.cpp). HEADER CHANGE: the Murod Oni's state
0x248c004-0x248c30f is one object, struct murod_oni_boss murod_oni (as the Titus's D8100).** The binary reloads
murod_oni_offense_point_count after each offense-point store (murod_oni_boss_load 0x4c5d2c) and
murod_oni_area_hit_count after the area-hit stores, which 8966 does for fields of one object only;
init_murod_oni_timers (0x4c5c30) constructs the threat timers and the four state timers in one function (D6510).
Extent: engaged first, area_hits last (the move pointers 0x248bff0 / 0x248bff4 / 0x248c310-0x248c318 and the unused
words 0x248bff8-0x248c003 stay separate globals). Field names ours, the old names are macros (also
murod_oni_threat_table and murod_oni_current_move, whose externs in the common section became comments).
LAYOUT_CHECK 0x30c. Won: murod_oni_boss_load, murod_oni_update_offense_points, murod_oni_collect_area_hits. Users:
boss_ai.cpp (51 matches before and after).

**D10504 (include/engine/ai/boss_ai.h, game/unnamed_evil_joseph_0x4c3470.cpp). HEADER CHANGE: the Evil Joseph's state
0x248b9b4-0x248bcf3 is one object, struct evil_joseph_boss evil_joseph (as D10503).** Same evidence:
evil_joseph_boss_load reloads evil_joseph_mesh_tag_count after each offense-point store (0x4c3650), and
init_evil_joseph_timers (0x4c3480) constructs the threat timers and the attack timer together. Extent:
combat_enabled .. the -1 word 0x248bcf0; the move pointers, the words 0x248b9a4-0x248b9b0 and evil_joseph_loaded
(0x248bcfc, after two move pointers) stay separate. LAYOUT_CHECK 0x340, old names are macros. evil_joseph_boss_load
also needs `name` / `ch` locals for the mesh-tag lookup (D7700) and `evil_joseph_loaded = true` before the two
unused 1 stores. Won: evil_joseph_spawn_helpers (with D10502), evil_joseph_boss_load. Users: boss_ai.cpp,
level_script_templeint2.cpp (no change).

**D10505 (include/vsdk/os/debug.h, vsdk/unnamed_debug_stubs_0x531c10.cpp). HEADER CHANGE: the display argument of
debug_return_zero and debug_noop_disp_rate is unsigned char.** All four callers of 0x531c50 (0x44191d, 0x441e39,
0x441f19, 0x4420e9) and the four of 0x531c90 in cmd_disp_rate load it with `mov cl / mov dl, byte ptr` and push the
whole register without widening (after rep movsb / a strncpy call the upper bytes are not known); an int parameter
makes 8966 emit `xor ecx, ecx` first. The callees ignore it (`xor eax, eax / ret`, `ret`); the definitions change
their signature only (the debug stubs unit still matches entirely). Won: mp_send_tracked; with D10500
multi_process_packet, multi_send_packet, multi_send_packet_buffered.

**D10506 (notes only; w6 pass). Tried, still differing.** file_list_files (the frame is now 0x150 with a 256-byte
directory buffer, as the binary; the slots of i / p / ext stay swapped under all 120 declaration orders and an
indexed loop); FileReader::read_string (8966 rotates the loop; the binary re-tests at the top); set_cipher_key and
eax_init (block placement); mp_handle_it_okay (8966 threads the second state test); pick_attack_slot /
claim_attack_slot (only the operand order of the first get_center x differs; plain, constructor and operator+
forms tried); ui_3d_begin (`vector3 p; p = light->pos + *pos;` gives the binary's frame 0x18 and is kept; the loop
counter init and increment are scheduled differently); mp_read_spell_targets / mp_handle_release_spell
(register choice, a late push); mp_reset_net_stats (a second zero register for the scalar stores).

**D10600 (game/new_inventory.cpp, game/new_charselect.cpp; worker 7). The clamp at 0 that ANDs into the value
register is `x &= (x <= 0) - 1` on the stored variable.** D7506 found no form for Sum.exe's `setle / dec / and
value, mask` (8966 gave `and mask, value` for max, ternaries, ifs, locals). Assigning the result to the global first
and clamping it in place with the bit form reproduces it: inventory_page_up (`inventory_scroll_pos -= page;
inventory_scroll_pos &= (inventory_scroll_pos <= 0) - 1;`) and mp_charselect_wrap_text_input
(`mp_charselect_scroll_pos[1] = rows - first;` then the same `&=`): both now exact. Same behaviour as max(x, 0).
The same form on a local, or after a min() into the global, does not help: inventory_identify_item,
inventory_destroy_selected_item and inventory_equip_item (min then max through `pos`) still differ only there.
Possibly a clamp macro in the original; the source form is a reproduction device.

**D10601 (game/new_inventory.cpp; worker 7). BEHAVIOUR FIX: inventory_handle_click skips buttons with def flag
0x10 in open mode; it did the opposite.** 0x415a0d-0x415a1d: `mov al, [inventory_open_mode]; test al, al; je
0x415aa8` then `test byte ptr [esi], 0x10; je 0x415aa8`, where 0x415aa8 is the press body (`or edx, 2` on the
button flags, pressed_button = i) and the fall-through continues the loop. So a button acts when
`!inventory_open_mode || !(flags & 0x10)`; the source had `(flags & 0x10)`. Flag 0x10 is on the screen and member
arrows (18) and the paper-doll hot spots (25): in open mode 1 (the unidentified-items list) they are inert, and the
scroll arrows, categories and close button stay usable. The table comment is corrected. The function still differs
(register choice from the first instruction).

**D10602 (worker 7). Store and argument orders that gave byte matches under 8966.**
- acm_convert: `h.pbSrc = src; h.cbSrcLength = src_len;` (8966 loads src_len first from that order).
- bm_replace_file: `w >>= 1; h >>= 1;` in the reduced case.
- ui_slider_base::get_marker_rect: no locals, stores `*my = 0; *mx = 0; *mw = w; *mh = h;` (found by trying all
  24 orders; one matches).
- gamecreate_frame: the field rectangle read into four locals before point_in_rect (D6205).
- inventory_hover_tooltips: the tooltip through `ui_button_state *b = &inventory_buttons[button]`.
- mp_player_disconnect: the append to the free list is the list_append macro (prev stored before next).
- Improvements without a match: nw_ras_init stores projection_size before projection.dwSize (only a cross-jump
  target differs now); gr_opengl_bitmap sets r, g, b to the constant 255 and a with an if / else (50 -> 25
  differing instructions, only the first corner's store scheduling left).
- Retried without success (8966): dead argument slots for mp_handle_status_add / snd_stream_fill_silence
  (block-scope locals, reusing the parameter), Crc32::update's register choice, wav_open's imul operand order,
  mp_send_obj_update's frame (declaration order has no effect), options_init's copy propagation of the first read.

**D10700 (vsdk/unnamed_gadget_0x52bfe0.cpp; m-1019 worker 8). LOGIC FIX, corrects D8200: ui_gadget::process
chains the children.** The load of key at 0x52c3a6 is outside the loop: the loop jumps back to 0x52c3aa and
pushes EAX, the previous child's result. So each child gets the previous child's result as its key (a child
that consumes the key passes 0 on), and the gadget returns the last child's result; a hidden or childless
gadget returns key (0x52c3c0). D8200 read the load as a reload per child and passed the original key to every
child. Now `for (; g; g = g->next_sibling) key = g->process(key); return key;`, a byte match.

**D10701 (game/living_entity.cpp, game/objects.cpp; m-1019 worker 8). /Od frames by the D6101 bucket rule,
again under 8966.** With the original frame read off the binary (pairing the k-th `[ebp+X]` reference of our
listing with the k-th of Sum.exe), locals were renamed so that their buckets ((h ^ (h >> 16)) & 15) are
non-decreasing from the top of the frame down, and declared bottom first (within a bucket the most recently
declared local is highest). A profile_label and block-scope arrays follow the same rule (the arrays of a block
keep their declaration order for their initialisers, so their names were chosen with strictly increasing
buckets). Matches: update_locomotion_anim, spell_apply_hit_effects (with D10702), update_regen_and_status
(with D10703), find_overlap (with its self test wrapping the rest of the loop body: 0x456b48 jumps to the
loop's end at 0x456e9a, not to the continue). load_state and item_obj_spawn now have every named slot right
but still differ (load_state: `test / setne al` for LE_GET_BOOL where 8966 gives neg / sbb / neg for every
form tried, D6107 (3); item_obj_spawn: the temporaries of the inline vector3 / matrix assignments).
scale_stats_to_level has every slot right after the two min() results became locals assigned with
ASSIGN_MIN (the binary's -0x18 / -0x1c are named locals written in both arms); it still differs only in
`(float)level / (float)info->level`, which the binary loads with fild and divides with fdivp where 8966 emits
fidiv in every form tried.

**D10702 (game/living_entity.cpp; m-1019 worker 8). spell_apply_hit_effects, spell 9.** The AP drain is
`add_ap(-(float)h->iamount)` (fild / fchs; the int negation needed a temporary, 4 bytes of frame) and the gain is
`if ((gain = (float)h->iamount) < 20.0f) gain = gain; else gain = 20.0f;` (fst into gain, then the
binary's self copy `mov eax, [gain]; mov [gain], eax`). Same results for every amount but INT_MIN.

**D10703 (game/living_entity.cpp; m-1019 worker 8). update_regen_and_status.** short_min is two returns, not
a ?: (8966 puts a ?: result temporary below `this`, an if / return one above it, where the binary has it, -0x30);
the AP gain is one expression `(int)(ap_regen_mult() * 0.05f * get_max_stat(1))` (the product spilled across
the call is a temporary, not a local), `ap = ap + regen` (movsx / add in int, not a 16-bit add), and the light
sum is `((float)cr + cg + cb) / 3.0f` (fild / fiadd / fiadd). Same results.

**D10800 (vsdk/targautils.cpp). tga_rle_decode clears the packet bit with `c &= ~0x80`.** The binary's byte
`and bl, 0x7f` on the int header (whose upper bits are known zero) is what 8966 makes of `&= ~0x80`; `&= 0x7f`,
casts and byte locals give `and ebx, 127` or other code. Same value. Now exact.

**D10801 (game/unnamed_render_scene_0x481680.cpp). level_vis_parse: the binary's buffers and compares.** The frame
is 0x1d4: set name 0x44 at +0, layer name 0x44 at +0x44, the "HIGH" name 0x4c at +0x88, the console line 0x100 at
+0xd4 (the source had 0x40 / 0x40 / 0x44 / 0x80; the console message always fit). The set-count limit is an
unsigned compare (`jae`), the layer record is indexed (`level_layer_vis[layer]`, ebx = layer * 12), and the final
loop walks a pointer (`jb` against &level_layer_vis[20].count). Same behaviour; still differs in register choice
(the binary keeps a zero in EBP and the found set in it).

**D10802 (game/unnamed_frame_0x4323e0.cpp). FrameEnd's locals by scope.** The 0x2c frame: the status message's
w / h at function scope (+8 / +0xc), the frame rate's own w / h (+0 / +4) and a 28-byte buffer (+0x10) in its block,
the mouse position (+0 / +4) in a block of its own around the rest of the function. 8966 lets sibling blocks share
slots; locals at function scope never do. Now exact.

**D10803 (game/unnamed_render_scene_0x481680.cpp). levelrender_level_load: 0x28-byte name, an unsigned light loop.**
The frame 0x2c needs a 0x28-byte name buffer. The one-time light loop keeps a biased pointer with `jb`: an index loop
bounded by `sizeof(levelrender_lights) / sizeof(levelrender_lights[0])` (unsigned compare; an int bound gives `jl`,
a pointer loop an unbiased pointer). Now exact.

**D10804 (game/unnamed_render_scene_0x481680.cpp). levelrender_draw_float_text: an 8-byte buffer, block-local
sizes.** The frame is 0x60 with the text buffer at +0x1c and the position at +0x24: `char buf[8]` (the "%d" and
"%d%%" texts of HP and stat changes; a longer text spills into p and v, which are written after the sprintf, so
it has no effect). w / h / hh are
local to each of the three text blocks, and drew_hp is cleared before gr_get_clip (`xor bl, bl` before the call).
Now exact.

**D10805 (game/unnamed_pfgeom_0x4a4ee0.cpp). pf_straight_walk_clear: `cur + hit` in the midpoint.** The binary
loads cur first (fld cur.x / fadd hit.x). Now exact.

**D10806 (game/unnamed_render_scene_0x481680.cpp). Loop bounds read once.** 8966 keeps a count across the calls
in a loop only when the source copies it: levelrender_add_character's `n = c->num_attachments` (count-down loop in
the binary); levelrender_add_lobjs reads the object table, the set's count and its index pointer once per set
(`lobj *base = lobjs; int n = set->count; int *p = set->indices;`) and tests and sets the flag through
`base[index]`. Both now exact. levelrender_setup_level_lights' delete loop as `while (i--)` (closer, still
differs).

**D10807 (include/engine/rendering/unnamed_render_scene_0x481680.h, game/unnamed_render_scene_0x481680.cpp).
HEADER CHANGE: levelrender_add_effect_entry's last parameter is int.** 0x4841e0 reads it as a dword (`mov eax,
[esp+0x30]; test eax, eax`); a bool is read with `mov al`. All three callers are in the same file and pass
`true` / `false` / a `!` expression (pushed as dwords in the binary). Strict sweep of the unit: no match lost; the
function still differs in the entry's store schedule. mingw: all 259 .cpp, no new error (unnamed_decals has 9,
as in the base).

**D10808 (notes only). Tried without result this pass:** key_flush (8966 turns the zeroing loops into rep stosd
in every form: while, chained, unsigned / short index), game_is_paused (bit field), journal_desc_page_up (seven
clamp forms), playmenu_frame, playmenu_open_save (the binary shares the save flag's slot with the button array;
block scopes, arrays: the bool keeps its own byte slot), m2v_skip_extension_and_user_data (`if (code != 0x1b5)`
gives the binary's layout up to the tail merge of the sequence display case), spell_fire_missile (register
choice), camera::update_eye_position (the squares now right with `d.x * d.x + d.z * d.z`; 8966 keeps sorting
`zoom * elevation` in every order), seg2d_point_near_middle (taking s->a / s->b into locals fixes the start; the
perp dot product's factors stay sorted), gr_get_frustum_planes, get_fade (prologue), levelrender_push_attachment
(the binary expands two of matrix::operator='s row copies and calls the third; inline_depth 0-3 tried),
levelrender_add_containers (sum orders, len_sq), level_vis_reset.

**D10900 (match-1019 worker 10: game/unnamed_utrack_0x48d7c0.cpp, game/new_gamelist.cpp, game/unnamed_fx_0x489c40.cpp,
vsdk/unnamed_gr_line_0x53e520.cpp). Six byte matches under cl 8966, strict; source shapes recorded.** (1) Branch
polarity: a test whose failure block the binary places last is written positive with the failure after it
(pxo_login_request `if (utrack_state == 1) { ... return 0; } return -3;`; mp_server_list_draw's level and players
colour tests `else if (ok) text; else red;`). (2) A switch whose jump table lists cases that go to the default
(pxo_login_request: table 0x48d9e0 over 1..5, entries 1 and 2 = default) is written with those cases spelt out
before the others (`case 1: case 2: return 0;`); left in `case 1: case 2: default:` they are dropped and 8966
emits a compare chain. (3) Nested handle tests with one shared `return -1` after them (fx_create_missile, as
D6205's ghost_seat_rider), and the direction as a vector3 difference (`dir = t->pos - c->pos`: the y and z
temporaries of the binary). (4) Rectangle arguments through locals x, y, w (D6205) and the row offset written
`row * font_h` (the induction variable is then the compiler's, set after the loop guard) in
gamelist_handle_click; its drag stores are `mode, thumb, start_y` (the thumb store comes first in the binary). (5)
Declaration order of initialised locals sets the order of their register initialisations: `ubyte codes_or = 0,
codes_and = 0xff;` (gr_clip_and_project_poly, gr_poly_project_to_screen; the latter also reads the vertex
through `gr_vertex *v = list[i]` as its sibling does). Behaviour unchanged in all six.

**D10901 (notes; game/unnamed_avoidance_0x4a6710.cpp). The vector2 operators in avoid_prim_from_segment and
avoid_poly_nearest_edge_hit.** The binary's centre, axis and hit point go through vector2 temporaries (y stored to
a stack temporary, copied with an integer move), which `out->centre = 0.5f * (*a + *b)`, `out->axis_a = *b -
out->centre` and `*hit = *p + t * (*q - *p)` reproduce (same arithmetic, IEEE-identical). avoid_prim_from_segment
then differs only at axis_b, which the binary constructs in place (fld x, fld y, fchs, two fstp): an explicit
`out->axis_b.vector2::vector2(-y, x)` gives a byte match with VC6 but is not C++ that GCC accepts, so the
assignment `out->axis_b = vector2(...)` stays (8966 builds a temporary and copies it, D6306 (1)). Not marked exact.
cmd_avoidance's type line is an if / else of two calls (the binary cross-jumps their tails, D6005); the function
still differs because 8966 threads the help branch's `doing_help_for_comand = 1` past the help test (the binary
jumps to the test).

**D10902 (game/unnamed_decals_0x4bbe80.cpp). decals_collect_for_lobj: the binary's tests.** `jae` after `cmp eax,
0x40`: the list-count test is unsigned, `(uint)*count < 0x40` (as shadow_collect_for_lobj, 0x4a4b40); the bit is
read as `(bool)d->proj.inside` (dword load, shr 4, test cl, 1); the loop is `if (d) do { ... } while (...)` with
one return of found (the binary has no early return). Same size now; d and count still sit in EBX / EDI swapped.
A negative count is not reachable (the caller starts it at 0).

**D10903 (vsdk/unnamed_http_0x55a8e0.cpp). http_get::send_get's block order.** The binary places the error
blocks after the loop in the order loop exit, read_data_channel, `code != 200`, `HTTP/` prefix: the nested form
`if (_strnicmp(...) == 0) { ... if (code == 200) { loop; error } else not-found } else error` gives that order,
and the Content-Length branch is `line = strchr(line, ' ') + 1; p = line;` (ESI walks, EDI keeps the start).
8966 still merges the loop-exit error block into the final else (the binary keeps four identical copies), so the
function differs from 0x55ad9e on.

**D10904 (notes only). Evidence and dead ends of this pass.** (1) conversation::clear_speaker (0x4d7e10) loads
the speaker into ECX before calling empty_function: probably a non-virtual empty member of conversation_speaker
called on it (thiscall), folded into 0x4f13e0; not applied (it needs a declaration in conversation.h).
(2) boss_move_in_range_dispatch: with `float r = boss->info->attack_radius;` before the distance call the Luminar
case's loads match (r goes to the dead `boss` argument slot); the function still needs tiger_move_in_range and
giant_salamanka_target_in_reach as bool (D7603 (3), other workers' units). (3) cquat::log: `fx, sx, fy, sy, fz,
sz` declared and computed interleaved gives the binary's spills of fy / fz up to the three conversions; the
remaining `fxch` is FPU scheduling. (4) Tried without result: page_up's max-AND (mask forms, ternary, if),
avoid_spline_point_get / avoid_spline_get block order (five shapes), ls_ikaemostopint_init's constant in EBX,
decals_reset's next / prev store order (the macro written out both ways), decal_place's specialised head
removal, gr_line_3d's copies of p0 / p1 into frame slots, fx_create_spell_effect_facing's direction.

**D10200 (include/vsdk/snd/ds.h, vsdk/unnamed_ds_0x55bf90.cpp). HEADER CHANGE: ds_play_streamed's fourth parameter
(looping) is bool.** 0x55d2b0 passes it to snd_stream_start, whose third parameter is a bool (read as a byte,
`mov al, [esp + 0x14]` at 0x566cad), with `mov ecx, [esp + 0x20] / push ecx` and no `test / setne`; an int parameter
makes VC6 emit `setne` for the bool conversion (D6202 is the same pattern). The only callers
(unnamed_snd_0x5204c0.cpp, 0x420 / 0x443 region) pass the constants 0 and 1, which compile the same; that unit's
results are identical with the old and the new header, and a whole-tree strict sweep loses no match. The priority
argument is passed on to ds_find_free_channel as its raw bits (`*(int *)&priority`, the binary pushes the dword
unconverted); the callee does not read it, so the behaviour is the same. ds_play_streamed now matches.

**D10201 (m-1019 worker 3; notes only). Source shapes that made VC6 8966 emit the original code in this pass.**
- An inline `vector3::operator==` (`level_ambients[i].pos == *pos`) instead of the three written compares gives the
  binary's loop layout (found block first, `jge / jmp` at the loop end): level_ambient_find_at; in goal_find it gives
  the binary's biased pointer (the loop pointer at pos.z), the function still differs in its loop tail.
- Case bodies in the binary's layout order: script_editor_update_buttons (none / change / edit / both),
  mp_level_find_boss (Azha's case before Pyrul's); a case that shares the final `return true` breaks out of the
  switch (pyrul_cast_special, with `default: return false`).
- Shared tails written as gotos: mp_level_find_boss (`goto found` for the level search, `goto boss` from every
  boss loop to one `mp_level_boss = c`).
- Branch order: the failure message as the else of `if (tf.SkipPast(..)) { .. }` (level_items_parse_table), the body
  inside `if (event == UI_EVENT_PRESSED) { .. } return false;` and `if (atoi(..) == 0) false; else true;`
  (script_editor_params_ok_cb); compare constants first, `0.0f < v || 100.0f > v` (script_editor_creature_ok).
- Element access: a local `s3d_named_path *p` for the trigger path (level_trigger_trip); the slot fields through
  `item_page_slots[slot]` instead of the slot pointer (item_page_in), the selection read through the global rather
  than a local pointer (script_editor_creature_ok: it gives the binary's `[edi + esi]` operand order), the first two
  channel stores through `ds_channels[ch]` (ds_play_streamed).
- Statement order: the pointer increment after the stores (`*dst = t; ...; dst++`, gr_clip_plane), the swap as
  `tmp = *p0; *p0 = *p1; *p1 = tmp` (gr_clip_line).
- Frame layout: two buffers where the binary has two (script_editor_list_door_sounds: an 8-byte one for the "%.1f"
  texts at +0xc and a 16-byte block-scope one for the lock percent at +0x14), and one shared buffer where the binary
  shares it (level_script_parse_table: the door key is read into `name`, the #Clicks buffer; 256 bytes less frame,
  the function still differs).

**D10202 (vsdk/frametime.cpp). frametime_set_limits copies b as an integer.** The binary loads a on the FPU and
moves b with `mov eax, [esp + 8]`; `*(int *)&max_reported_frametime = *(int *)&b;` (and the same for the minimum)
gives it. Same behaviour; now exact.

**D10203 (m-1019 worker 3; notes only). Not reproduced in this pass.** pathvis_add_edge_node (the list unlink through
`&e->prev` / `&e->next`, D7006; a local pointer pair is folded away), matrix3_pack_orientation (the first pair in the
dead argument slots of out and m; block scopes give the 8-byte frame but not the slots), ds_is_3d_buffer (`shr al, 4
/ and al, 1` from a dword load; bit-field and cast forms tried), script_editor_default_action_navpoint (the NULL kept
in ESI), automap_center_on_player (8966 sinks the two conversions into the centred branch), pyrul_fire_attack (the
binary calls vector3's constructor 0x43b580 out of line), and register-choice differences in item_page_out_all,
bbox_silhouette_corners, mp_player_file_create, script_editor_select_prev.

## m-1020 worker 1 (D11000-D11006): vsdk quaternion / vfx emitter / gr_font / exception handler / sound emitter / movie / console, game fx / shadows / item trade / buy-sell / snow / alt textures / gamesound

**D11000 (vsdk/unnamed_exceptionhandler_0x5419a0.cpp). In an EBP frame (here the SEH frame of an __try function)
8966 orders the locals by D6101's name hash under /Ox too; the article's local names give the binary's slots.**
RecordExceptionInfo 0x5419a0 and ShowModuleInfo 0x541fb0 differed only in frame slots (context at ebp-0x24 and
the crash module name at ebp-0x28 in the binary, disp8 operands; ours were further down, disp32, so every later
offset moved). Renaming one local moved the slots of the others, which D10802 had not seen for /O2 ESP frames
(there the names do not matter; gr_font_create's free_slot stays put when renamed). The unit follows Bruce
Dawson's ExceptionHandler.cpp (file header), so its local names were taken over (ModuleName, FileName, FilePart,
lastperiod, LogFile, Exception, Context, CrashModulePathName, CrashModuleFileName, MemInfo, code, codebyte, pStack,
pStackTop, Count, buffer, safetyzone, nearend, output, Suffix, DebugMessage, readwrite; ModName, DosHeader,
NTHeader, ModuleFile, TimeBuffer, FileSize, LastWriteTime) and the locals are declared where they are first used
(the "Unknown" name buffer after the been-here test: the binary tests the flag before it copies the literal). Both
now match. Device for other EBP-frame functions: names decide the slot order (D6101's bucket formula).

**D11001 (game/gamesound.cpp, vsdk/unnamed_movie_0x5172f0.cpp, vsdk/unnamed_sound_emitter_0x546150.cpp). Early
returns in EH functions; an unlink through the element's own fields.**
- surfaces_parse_table 0x468e30: `if (!file.find(..)) return;` instead of the if-block; 8966 then keeps 0 in EBX for
  the whole function (EH state stores, num_surface_types = 0) and pushes all registers in the prologue, as the
  binary does. Same size (the destructor path is shared). Not general: alt_textures_parse_table with an early
  return duplicates the destructor tail (972 bytes against 933).
- movie_play 0x5172f0: `if (os_get_main_wnd() == NULL) return;`, then `char path[MAX_PATH]` declared after it; the
  File object then sits at [esp] as in the binary (D5607's "File above the path whatever the order" was the
  if-block form).
- sound_emitter_delete 0x5467c0: `e.prev->next = e.next; e.next->prev = e.prev;` written on sound_emitters[emitter]
  directly (no prev / next locals) gives the binary's store schedule (D8701's open point).

**D11002 (vsdk/console.cpp). `je` to a `ret`, then `jmp` to the next aligned address, is a tail call; 0x50ab70
is a function of its own.** init_char_types 0x50ab60 is `cmp byte [0x25db91c], 1 / je ret / jmp 0x50ab70 / ret /
nop`; the table builder at 0x50ab70 (not in sum.symbols: nothing calls it) is a separate static function
(build_char_types, name ours) that the /Ob0 unit reaches by a tail jump, as set_command_line's jump to
get_next_char. Both match (one function more in the compared set). Look for this shape elsewhere: a short
function ending in `jmp` to the next 16-byte boundary.

**D11003 (vsdk/gr/gr_font.h, vsdk/gr_font.cpp; header change). gr_string and gr_string_at_text_pos take gr::Mode
by value, not int.** gr_string loads its mode argument (`mov ecx, [esp + 0x34]` at 0x508d13) and pushes it to
gr_bitmap_ex (0x508d3e) with no call of the Mode(gr_mode) constructor 0x506810; with `int mode` VC6 must call it
(ours did, and the extra temporary cost the frame: push ecx, the y / cx locals in memory). gr_string_at_text_pos
0x508dc0 passes its argument through the same way. With gr::Mode, gr_string 0x508c60 matches (its dead argument
slots then hold c, width and spacing as in the binary) and gr_string_at_text_pos still does. gr_font.h
forward-declares `namespace gr { class Mode; }` (items.cpp and plainstring.cpp include it without gr.h). All 259 call
sites pass gr_string_mode or gr_bitmap_wrap_mode (gr::Mode globals); a full strict sweep loses no match.

**D11004 (game/unnamed_snow_0x4946a0.cpp). snow_update 0x494ba0 reads snow_cells[7][7] at each use.** The binary
loads the centre cell after the reset-flag test (`mov edi, [0x2445388]`) and again after the x scroll for the z
tests; a `centre` local read once before the test put it in EBP and shifted everything after. Also `float fx =
fvec.x; float fz = fvec.z;` for the horizontal length (the sum of squares of the members loads z first under 8966,
the binary loads x first: a strict `target` difference only), and the flake step written on
`snow_cells[i][j]->flakes[k]` twice instead of through a flake pointer. Now exact. Behaviour: after an x scroll the
z comparison uses the new centre cell, which has the same z as the old one, so nothing changes; the +x refill bug
(NOTES.md, "update returns true") is confirmed by the byte match.

**D11005 (game/new_itemtrade.cpp, game/new_buysell.cpp). The list drawers and page-up functions.**
- itemtrade_party_page_up 0x419bb0, buysell_page_up 0x405790: `pos -= page; pos &= (pos <= 0) - 1;` on the global
  (D10600's form).
- itemtrade_draw_list1 0x41a360, _list2 0x41a490, _party_list 0x41a5a0: `int rows = count < page ? count : page;`
  (the binary stores rows to memory on both paths) and the row index in a local `int row = scroll + i` before the
  element access or call (gives the binary's y / i registers).
- buysell_draw_item_list 0x405fd0: the same ternary, and `for (int i = 0; i < rows; y += fh, i++)` (the y step in
  the for header, before i: with it in the body or after i the registers or the two adds swap).

**D11006 (m-1020 worker 1; notes only). Not reproduced (forms tried in w1-NOTES.md).** itemtrade_count_party_rows
(the re-read of party_gold after the row-count store, 5 forms; with D11305's volatile store the load order matches but the increment stays in EAX), buysell_count_party_items (the max-0 AND register
after a min: 26 min / max forms), snow_start (outer record pointer stepped at the loop top and the loop ended on the
cell pointer: 15 loop shapes), alt_textures_parse_table (TextFile now declared after the table reset, as the binary
constructs it; the shrink-wrapped EBX / EBP pushes remain), quaternion::to_matrix / _b (8966 orders the factors of
each product by a rule no source form changed: 2 factor orders, statement orders, 24 orders of local copies),
quaternion::multiply (the r.w / r.z store order: all 24 orders of the four stores), quaternion::exp / cquat::exp /
exp_in_place / log_in_place (the binary keeps the square root on the FPU stack (`fstp st(3)`, then `fst [esp]`);
the interleaved fx / sx declarations are kept: they match up to that point), fx_create_spell_effect_facing (`vector3
*tp = &t->pos` and `dir.set(..)` match up to the right vector, where the binary loads 0 then dir.z and uses
fsubr), fx_recb0::init_particle (the binary converts desc->frames with fild after the frand_range call and adds with
faddp; every form gives fiadd or keeps the random value on the stack; the two shape tails are merged by 8966 in our
form too), fx_create_spell_area (shrink-wrapped pushes), fx_update_missile (the binary calls vector3's operator+,
operator= and copy constructor out of line here (and in fx_frame, fx_find_spawn_point); written with vector operators ours still
expands them), the handle_click functions, shadows_render (lobj_count loop counted down), and the large fx / vfx /
shadow functions.

**D11100 (m-1020 worker 2; game/unnamed_decals_0x4bbe80.cpp, game/pathfinding.cpp). Loop entry tests.** Where the
binary tests a list head before the walk (or not at all), the loop form decides it under 8966:
decals_recycle_unused (0x4bcc20) is `if (decal_active) while (d) {...}` (the binary tests the loaded head, then
copies it to d); decals_collect_for_lobj (0x4bcbb0) is a top-tested `while (d)` whose step is `d = d->next !=
decal_active ? d->next : NULL` (as recycle), not a do-while with a two-part exit (this also gives D10902's
register assignment); ai_path_smooth (0x49f650) walks the layers with a do-while (path is non-NULL there, the
binary has no entry test). All three exact; behaviour unchanged.

**D11101 (m-1020 worker 2). Early-return and goto blocks placed as in the binary.** 8966 places a `goto found` /
`goto fail` pair in source order the wrong way round, and an early return in a then-branch inline. Forms that
give the binary's layout: a found flag set before `break` and the failure as `if (!ok) { ...; return false; }`
(murod_start_leap 0x4c51a0; luminar_teleport_out 0x4c9da0 with a second `busy` flag so that the busy return
shares the failure's tail); the main path in the then-branch with `else return 0;` (murod_ai_select_action
0x4c57b0: the binary places the return out of line); an `opened` flag so that both open failures share one
`return 1` (tga_read_header 0x555110). Also murod_start_leap's direction is `float z = frand() - 0.5f;
dir.x = frand() - 0.5f; dir.y = 0.0f; dir.z = z;` and its leap direction `t->pos - dir`, find_move's result in a
local (D10000). All five exact; behaviour unchanged.

**D11102 (vsdk/unnamed_vector_0x523f30.cpp). vector3_rotate_about_axis passes the angle through a float local**
(`float a = angle; matrix spin(0.0f, a, 0.0f);`, fld / fstp into the argument, as D10307). Exact.

**D11103 (game/unnamed_decals_0x4bbe80.cpp). decals_reset: plist_append written out, the prev store through a
volatile lvalue** (`*(decal_record * volatile *)&d->prev = decal_free->prev;`), which keeps it above the next
store as in the binary (the coordinator's D11305 device; D10904 had tried both plain orders). Exact; a
reproduction device, behaviour unchanged.

**D11104 (m-1020 worker 2; notes). Closer, still differing (kept, behaviour identical).**
- items_parse_table (0x44a090): 888 -> 29 differing lines. `if (lang[0])` guards the tolower loop; the type
  slot is `if (num >= MAX) t = &dummy; else t = &item_types[num++];`; the skill / resist mod values through
  `float f = value * 0.01f;` before the Alloc; the 9 status bytes cleared by a loop (the binary's direct
  stores; memset gave a lea); `value = 0` / `c = -1` before ReadLine; the charge effect's self-only flag is a
  bool set per branch with a `charge` flag (no `charge == 2`); the slot switch has cases 6-8 before 2-3; the
  armour texture and description copies store the pointer before strcpy; ITEMS_STRDUP is a plain block, not
  do / while (0) (the latter kept the model store above the next call's `this`). Left: register choice in the
  four armour-texture case blocks (all 24 case orders, `+ k` and flat-index forms tried).
- cutscene_parse_table (0x4355e0, /Od): the 16 locals renamed for D6101's buckets (frame 0x288 now as the
  binary) and the shot-effect length ternary in short (`(short)1`, `(short)0`). Left: from the $Fog count
  increment on, the /Od register rotation is one step ahead of the binary's (five forms of the increment tried).
- serpent_illusion_update (0x496d60): `int base = 255; if (i != 0) base = 230;` and the same form for the
  third block's alpha fix the register and frame use. Left: 8966 turns `base + x / -1000` into `base - x / 1000`
  (six spellings) and drops the binary's `rem <= 300` re-test.
- combat_toggle_ready_stance (0x4bdce0): `volatile bool flip` gives the binary's frame slot, entry store and
  load (the value is never otherwise read: the binary reads it as memory, D11103's device); the prop switch as
  an if chain on the move id. Left: prologue order and `ready` in AL instead of BL.
- Tried without result: seg2d_closest_point's 8 unused frame bytes (12 forms), pf_find_cell_crossing's frame
  (vector forms; one gives the frame but spills), choose_support_spell's EBX / EBP swap (declaration orders),
  vector3_angle_and_axis (acos result store placement), luminar_boss_load's two lengths (sum / store orders),
  combat_cast_frame's give_up block (label inside the first test; expanded copies), decal_place's head
  removal (`d = decal_free` after the recycle), serpent_part_is_busy_or_asleep through an inline helper,
  cmdline_take_argument with a volatile is_done store, vector2_dist_sq_to_line (dot orders: `ap.y * ab.y +
  ap.x * ab.x` gives the loads, the `fadd st, st(1)` remains).

**D11200 (vsdk/gr_opengl.cpp; m-1020 worker 3). The 16.16 resampling converters: the row pointer is computed from y,
the row padding starts at x = bm_w.** The binary computes the destination row inside the bm_h guard as an induction
variable (`mov edi, [dst]` after `jle`, `add edi, 4 * tex_w` per row) and, after the texel loop, reloads bm_w and
fills `tex_w - bm_w` dwords (`cmp esi, edx / jge / sub ecx, esi / rep stosd`); the padding rows start from
`dst + bm_h * tex_w` inside the tex_h guard. `uint *d = (uint *)dst + y * tex_w;` per row, `for (x = bm_w; x < tex_w;
x++)` and a per-row `uint *pad = (uint *)dst + y * tex_w;` give it (the old `row` pointer and the `for (; x < tex_w;)`
continuation do not). gl_convert_888_to_rgba8888_scaled also zeroes b, g, r, a in that order (the binary's ECX, EDX,
ESI, EAX). Won 0x54d630 and 0x54dda0. Same results except for a negative bm_w, where the source now pads as the
binary does. The same shape brings 0x54d8d0 / 0x54da80 / 0x54dc20 to the right frame; they still differ in how the
source texel address is formed (the binary materialises `src + 2 * index` with a `lea`, a dead one in 0x54da80: no
pointer, cast, byte-pointer or `*s++` form reproduced it). 0x54da80's alpha is an int (`neg / sbb / and cl, 1 / add
ecx, 0xff`), not a ubyte.

**D11201 (vsdk/gr_opengl.cpp). gl_convert_4444_to_rgba4444 / gl_convert_1555_to_rgba5551: the destination pointer
is loaded after the tex_h test** (`test eax, eax / jle` before `mov ecx, [dst]`). Only `int y = 0; if (tex_h > 0) {
ushort *d = (ushort *)dst; do { ... y++; } while (y < tex_h); }` gives it; the for loop with d declared anywhere
before it, a for-init `d = dst`, an early return, `d[x]` row indexing and `*s++` forms load dst before the test.
Same behaviour. Both exact. The same form makes gl_convert_a8_to_rgba8888, _888_to_rgba5551 and _8888_to_rgba4444
worse (register choice), so they keep the for loop (a8's increment is now `y++, src += bm_w`, closer).

**D11202 (vsdk/gr_opengl.cpp). glOrtho's arguments: `offset_x + clip_left`, `clip_left + offset_x + clip_width`,
`clip_height + offset_y + clip_top`, `clip_top + offset_y`.** 8966 reassociates the sums; of the 144 operand orders
of the four arguments (tools: a generated vc6-try variant list) 12 give the binary's bytes and only the `clip_top +
offset_y` top gives its load order (offset_y first, strict). gr_opengl_set_viewport and gr_opengl_init_context (the
same block) both exact; init_context also declares its PIXELFORMATDESCRIPTOR in the first-call block (frame 0x3c as
the binary: the block shares the ortho temporaries' slots). Same values.

**D11203 (vsdk/unnamed_lights_0x51e070.cpp). Selection loops read the depth once; signed type tests through an int.**
lights_shade_vertices: `int cur = lights_selection_depth - 1;` for the count and the row (the binary keeps a
pointer to the row and a count-down counter across lights_shade_point), and `lights_shade_light_total += n;` before
`lights_shade_calls++;` (store order). lights_select_nearest: `int p = lights_selection_depth - 1;` before
`d = lights_selection_depth++;` (the binary's `lea eax, [ecx - 1]` before `inc ecx`) and `int type = l->type;` for the
range test (movsx / sar / `cmp eax, 2` / `cmp eax, 3`; the field compared directly gives `cmp ax, 8` on the shifted
word). Both exact. lights_calc_point_specular_color uses the same `cur` read and tests the type with `switch (l->type)
{ case LIGHT_TYPE_POINT: break; default: continue; }` (the binary's `sub eax, 2 / jne`), `normal == NULL ?
dist * 0.1f : normal->dot(d)` (8966 places the first arm of `normal ? a : b` first whatever the polarity; written
NULL-first the binary's `jne` to the dot call comes out), the clamp in two statements (`s = (s - threshold) * (1 /
(1 - threshold)); s = float_max(s, 0.0f);`: the binary stores s and pushes it) and `(s * s) * atten * intensity`.
It still differs only in where the reflection's `2.0f * *normal` temporary address is loaded (8966 loads it before
the dot product's fstp). lights_shade_point: the same NULL-first ternary in both cases, `vector3 t = *pos - lp;
vector3 v = t.unrotated(gr_light_matrix);` (frame 0xb0 as the binary; the difference is computed before the matrix
is pushed); it still differs in the slot of dot and the store of the first acos result (the binary stores ang_u after
the next call's address loads). Same behaviour throughout.

**D11204 (game/unnamed_cutscene_script_0x4ed050.cpp). A call result kept in a named local is compared with the zero
register.** The binary keeps 0 in EBX and tests strstr's and TextFile::Open's results with `cmp eax, ebx`; `char *ext
= strstr(...); if (ext == NULL)` and `int err = tf.Open(...); if (err != 0)` give it (the expression forms give `test
eax, eax`; with `== NULL` too). cutscene_script_load still differs: the binary places the shared failure tail
(TextFile and File destructors, `xor al, al`) right after the level-name test, 8966 at the end, for the plain, nested,
goto and local-result forms.

**D11205 (notes only; m-1020 worker 3). Tried without a match:** gl_convert_a8_to_rgba8888 (EDX / EBP swapped for
tex_w / bm_w in six loop forms), gr_opengl_apply_tint (the binary keeps the sum on the FPU with `fst`; only an
assignment inside the first product, `(sum = ...) * gr_tint_r`, gave that, and 8966 then still loads the tint
before sum for the green and blue products: reverted), gr_opengl_read_backbuffer (the binary keeps two row pointers
stepping by +-4w and copies them for the inner loop; swap orders, row-pointer and indexed forms tried),
bm_load_internal (the binary keeps `reduced` in BL and the constant 1 in EAX; bool / int / ubyte and the M2V test
polarity tried), bm_lock_pcx (`lea esi, [eax + 8 * eax]` with `[8 * esi + field]` addressing where 8966 shifts the
index; operand order and a bpp local tried), ghost_anim_remap_in (zero register and ready kept in the c argument
slot; declaration orders and the position of `ready = 0` tried), state_game_enter (the default case's body is placed
after the level_load failure block in the binary; default first / middle / last and a pre-switch `new_game = false`
tried), state_game_frame (the binary copies camera_fov and the frame times into a float local with integer moves
and pushes the register; a float local gives fld / fstp), float_format_buf (the binary tests rem from memory after
an integer copy of a and keeps a guard before the x loop; `*(int *)&rem = *(int *)&a` gets the copy and the guard,
the guard then reloads a instead of `fld st(1)`; not kept), s3d_read_mesh_ref (the binary reads hi as a dword and
masks it with 0xff before the shift, which `int hi` reproduces, but the field store order and the box copies through
two pointers still differ; reverted), light_create_spot (the clamped intensity's fstp is scheduled after the type
store).

**D11300 (game/new_assess.cpp, game/new_pxochat.cpp; m-1020 worker 4). The clamps at 0 on a global, both
directions.** D10600's `x &= (x <= 0) - 1` in place on the global gives Sum.exe's `setle / dec / and value, mask`:
assess_page_up (`assess_scroll_pos -= page; assess_scroll_pos &= ...`) and pxochat_init (the chat list's last page).
pxochat_add_chat_line has the other polarity, `sets / dec / and`: `pxochat_scroll_pos[2] &= (pxochat_scroll_pos[2] <
0) - 1` (the binary also zeroes ECX with `mov ecx, 0` there, which this form gives). Same behaviour as max(x, 0). All
three now exact.

**D11301 (vsdk/unnamed_os_0x530b90.cpp). PrintSystemInfo and cmd_sysinfo: /Od frames by the D6101 / D7003 bucket
rule.** The binary's slots from ebp-4 down are mhz, katmai, ext[0x400], mmx, tsc, amd3dnow, name[0x80], ms, family,
mb, dst, speed, src, count, freq, per_copy, end_time, start, stop (the source's declaration order); 8966 orders /Od
locals by name bucket, so the locals are renamed to buckets in that order (cpu_mhz 0, katmai 1, text_buf 2, tsc_val /
mmx_flag 3, amd_val 4, name1 6, memstat 7, fam 8, ram_val 9, dst_v 10, speed3 11, src_v 12, freq8 / copies_v 13,
end_time_ / per_copy2 14, stop_b / start7 15), the two of one bucket declared in reverse frame order. cmd_sysinfo's
TextBuffer sits above its 2 KB buffer in the binary: `buffer` (bucket 11) instead of `buf` (2) puts it after `tb` (9).
Both exact; the names are a reproduction device only (tools: w4-NOTES).

**D11302 (vsdk/gr_glide.cpp). glide_record::evict_block reads the block's texture and section once.** The binary
keeps both in registers across the first store through the section table; `int tex = b->texture; if (tex > -1) {
int sec = b->section; ...}` gives it (the source re-read them after the store). Exact. glide_record::load_section:
`info.largeLod` is filled before `info.smallLod` (the binary reads +0 before +4), and the scales go through float
locals (`float u = sec->u_scale; float v = sec->v_scale;`): u stays on the FPU as in the binary; still differs in
the schedule of the int copy of v (the binary keeps v in the dead `section` argument slot and copies it after the
bitmap store).

**D11303 (game/level_script_worldmap.cpp). ls_worldmap1_frame: the saved position goes through a parenthesised float
temporary.** The binary pushes each coordinate as `push ecx / fld [pos] / fstp [esp]` (a float argument through the
FPU), while the heading (a float local whose address is taken) is pushed as an integer copy. `{ float t =
(cur_character->pos.x); savegame_state.set_float(..., t); }` per coordinate gives exactly that (D10402's parenthesis
effect, here for an argument). Exact.

**D11304 (game/unnamed_frame_0x4323e0.cpp). screenshot_save: the pattern choice is an if / else of two calls.** The
binary pushes 0 and 1 before choosing the pattern (`test al, al / lea eax, tga / jne / lea eax, jpg`) and then pushes
path_root and the list: 8966 tail-merges `if (targa) file_find_files(.., tga, 1, 0); else file_find_files(.., jpg,
1, 0);` into that one call; the ternary argument is evaluated before the pushes. Exact (D10100's ternary-argument
rule again).

**D11305 (vsdk/gr_glide.cpp). gr_glide_init's glide_init_done stores are volatile accesses.** The binary keeps
`mov [glide_init_done], 0` ahead of the SetEnvironmentVariableA pushes, and the four clip-rectangle stores and
`glide_init_done = 1` ahead of grHints' pushes, while `glide_swap_enabled = 1` is sunk past them; 8966 sinks every
plain global store past the next call's argument pushes, and keeps a volatile store (and the stores before it) in
place. `*(volatile int *)&glide_init_done = 0 / 1` at the two stores (no header change: the variable is written
only here and never read) gives the binary's order. Exact. Probably the original declared the flag volatile (it may
have been polled by the window procedure in an earlier version); worth trying wherever a global store stays above
pushes that 8966 moves it below. /Oa, /Oy- and other flag sets were tried first for the unit: no gain.

**D11306 (game/conversationctl.cpp). conversation_controller::select_topic: per-flag bool tests, inline max / min,
array indexing.** (1) The binary tests `farewell` and `shop` with two reads of +4 (`mov ecx, eax / shr ecx, 2 / test
cl, 1` and `shr eax, 6 / test al, 1`); `topic->farewell || topic->shop` lets 8966 merge them into `test byte [+4],
0x44`, `(bool)topic->farewell || (bool)topic->shop` keeps them apart (the file already reads farewell as (bool) in 0x4dac30).
(2) The scroll clamp is the inline max then min macros (cmp / jle / mov, then min(x, 0) as `setge / dec / and`), not
the out-of-line imax / imin (0x51b750 / 0x503dc0): `int lim = font_h * lines - text_height; lim =
max(dialogue_text_box.scroll_target, lim); dialogue_text_box.scroll_target = min(lim, 0);`. (3) The scroll search
indexes dialogue_text_box.line[idx] each time instead of through a pointer. Exact. conversation_controller::frame:
`if (r != 0 || cam_mode == 3) return r;` (the binary returns the zero r for cam_mode 3 and pushes the known-zero
EAX as distance_between_edges' false); still differs in the frame (0x48 against 0x40).

**D11307 (game/new_pxochat.cpp). Loop and statement shapes.** pxochat_add_typed_channel: the slot reuse is inside the
search loop (`if (lobby_channels[i].name[0] == '+') { copy; lobby_flags |= 8; return 0; }`), not a break and an
`if (i < lobby_num_channels)` after it (the binary has no second count test). pxochat_build_popup_text: `len = 0`
after the NULL test (EBX is zeroed there) and `len += lengths[i]` before each strncat (the binary adds the loaded
length while pushing it). pxo_parse_chat_banners_cfg: `int num_found = 0` after the -1 fill (the downloaded_only part
now differs only in the EBX / EBP swap of count and the constant -1 that runs through the whole function). All but
the last exact.

**D11308 (game/new_pxochat.cpp). The visible row count of the two lists is an if / else of two assignments.**
`if (pxochat_list_count[n] < page) rows = pxochat_list_count[n]; else rows = page;` gives the binary's store of the
count, compare, store of the page, and the reload of rows from its slot before the loop; `rows = count; if (rows >=
page) rows = page;` (and min / ternary forms) keep rows in a register. pxochat_draw_channel_list and
pxochat_draw_user_list exact. The same `rows >= page` shape is in other units' list draws (buysell, charselect,
gamelist, inventory, itemtrade): worth trying there.

**D11309 (game/new_pxochat.cpp). pxochat_update_scrollbar.** page is read before count, the empty-list case is the
then-block (`if (count <= 0) {...} else {...}`: the binary falls into it and jumps to the thumb arithmetic), and the
thumb position is written `track_h * ((float)scroll_pos / count)` so that count is loaded first and kept for the size
term. Exact.

**D11310 (notes only; m-1020 worker 4). Tried, still differing.** ui_window::send_event (the binary returns the
callback's EAX or the NULL callback itself: a void / int callback, header question across the ui units, not changed);
game_is_paused (`shr al, 2`: byte forms of game_flags give a byte load); pxochat_page_down / drag_scrollbar, ParseInt /
ParseUInt, banners_cfg (register choice; declaration orders and local forms tried); line_talker (the binary uses EAX /
ECX for the temporaries where 8966 uses ECX / EDX; block-scope and return forms tried); os_wndproc (a walking handler
pointer gives the binary's loop and prologue; the binary cross-jumps the two SetThreadPriority calls, which no
switch / break form reproduced); ls_worldmap1_pick_encounter_stage9 (the desert-night block placed after the orenian
branch; condition and branch inversions tried) and _stage15 (/O2 frame slots follow use, not declaration order or
names); set_valid_chars / set_invalid_chars (the binary shares one byte slot for the three range-macro lo values and
expands each memset in place; function-scope lo / hi and pointer forms tried); camera_capture (vector3's constructor
called out of line, D9807); gr_glide alloc, tcache_set, create_texture* (register choice / dead argument slots).

**D11311 (vsdk/parse.cpp). TextFile::Error: the loops as the binary indexes them.** The "found" copy indexes
`cursor[i]` directly (with a `char *p = cursor` copy 8966 strength-reduces the loop into a pointer difference and a
down-counter; the binary keeps i and `[esi + eax]`), and the "prior" loop is `for (i = 0; i < 40 && start !=
cursor; start++, i++)`: the binary tests start != cursor at the top and i < 40 at the bottom (the rotated form of
that order) and increments start before i. Same behaviour; exact.

**D11400 (m-1020 worker 5; experience, murod_oni, gr_3d, linefile, key, pfgeom, avoidance, gamelist). Source shapes
that made cl 12.00.8966 emit Sum.exe's code** (each a strict byte match unless said otherwise):
- A stepped sum as a closed form the compiler strength-reduces: xp_for_level `for (i = 20; i <= level; i++) xp += 50000
  + (i - 20) * 10000;` gives the binary's step register and its late `cmp edx, 20 / jl` (the `step += 10000` loop
  hoists the step store above the test in every order tried).
- Assignments instead of returns where the binary shares the value blocks across an if-chain: key_remap_layout assigns
  `code` in all four layouts and returns it once; layouts 2 and 3 are switches, layout 2's cases in the binary's
  block order (0x1e, 0x32, 0x2c, 0x27, 0x33, then 0x10 / 0x11, whose value blocks are cross-jumped into layout 4's
  and layout 3's). 64 combinations searched; the return form never merges the same way.
- Positive tests with the failure last: murod_oni_ai_select_action `if (handle != -1) set_timer; else return 0;`;
  LineFile::write nests the line-end selection on `cr != NULL`, then `lf != NULL`.
- A vector2 edge normal built first, then the dot product (seg2d_point_left_of, pf_bsp_node_find's line case):
  `vector2 n(b->y - a->y, a->x - b->x); (p->x - a->x) * n.x + (p->y - a->y) * n.y` keeps the normal on the FPU
  stack as the binary does (the float locals were folded into the expression).
- A float read again through a local: gr_offset_point_depth `float oz = v->z; s = z / oz;` (fld / fdivr); also
  `z / (v->z)` matches.
- gr_setup_frustum: `z` in a local, `-hx` / `-hy` as locals stored right after each quotient (the binary's fst / fchs /
  fstp pairs and its 8 more frame bytes), and `gr_matrix_scale.z * gr_far_clip_dist` (the strict check named the
  operand order).
- Indexing instead of a pointer local where the binary keeps the scaled base: avoid_seglist_ensure_corner_offset
  reads `sl->corners[corner]` each time (the binary spills `sl + corner * 0x44`, not the corner's address).
- The polygon edge loop: avoid_poly_nearest_edge_hit `for (i = 0, j = n - 1; i < n; j = i++)`, no early return for
  an empty polygon (the loop test covers it) and `return best != FLT_MAX;` (EAX set as a whole).
- mp_handle_actv_gm: `e->ping &= (e->ping <= 0) - 1;` for the clamp at 0 moves the version byte back into BL and the
  frame to 0x40 (closer; the list unlink through `&e->prev` and the AND register still differ).
- gr_setup_3d (closer): no `h` local (the clip height is converted twice and CSE'd, so `fmul aspect` works in place),
  the tangent argument through `float a = fov * GR_DEG_TO_RAD; tan(a * 0.5f)` (the binary multiplies twice; one
  constant was folded before). The Glide mode test is still threaded into one in our build.

**D11401 (game/unnamed_avoidance_0x4a6710.cpp, include/engine/ai/unnamed_avoidance_0x4a6710.h). The two miter
functions through vector2 temporaries; HEADER CHANGE: avoid_seglist_calc_corner_offset's swap is an int.**
avoid_seglist_calc_corner_offset (0x4a7e30) and avoid_seglist_calc_shared_corner_offset (0x4a8060) now match:
`d = corners[b].pos - corners[a].pos` (operator- temporary, copied), `vector2 n(d.y, -d.x)` constructed in place and
copied whole into seg->normal, the swap as `d1 = d2; tmp = n1; n1 = n2; n2 = tmp`, `n2.dot(d1)`, ONE float for the
negated cosine and the scale (the binary keeps both in the argument slot of seg), the half angle in float
(`ang = PI_F - (float)asin(s)` / `(float)asin(s)`, then `1.0f / (float)sin(ang * 0.5f)`: the binary's `fsubr dword` /
`fmul dword` against float constants), `vector2 bis = n2 + n1`, `bis.x = bis.x * s` and whole-vector stores of the
offset. The callee reads swap as a dword (`mov eax, [esp + 0x4c]` at 0x4a7eeb, then `test eax, eax`); the callers push 1 / 0,
so the declaration is `int swap` (the only caller, avoid_seglist_add_segment, passes true / false unchanged). PI_F
is 3.14159265f: the constant at 0x57f028 is 0x40490fdb (3.141593f gave 0x40490fdc; the strict check found it).

**D11402 (game/unnamed_avoidance_0x4a6710.cpp, include/engine/ai/unnamed_avoidance_0x4a6710.h). HEADER CHANGE:
avoid_sort_refs_along_segment takes (count, refs).** The callee loads its first argument as the loop count and its
second as the entry pointer (0x4a8aa6 `mov esi, [esp + 0x1c]` with three registers pushed is the second argument,
passed to avoid_ref_entry and walked by 12; qsort gets it as the base), and avoid_merge_shared_segments pushes the
entry array first and the count last (0x4a8c1a-0x4a8c22). Declaration, definition and the one call swapped; same
behaviour. The function still differs (EBX / EBP swapped between use_x and the count); `fabsf(dx) > fabsf(dy)` gives
the binary's load order.

**D11403 (notes only; m-1020 worker 5). Tried without result:** xp_level_up_gains (EDI load scheduling; capfirst,
group pointer, int species copy), options_init (the first setter reads options_edit after the rep movsd: memcpy,
order, pointer forms), options_load_bitmaps (ESI / EDI for i; filename scope, declaration order), options_drag_sliders
(the slider width copied once more: float_min / float_max for the clamps compiles to the same code),
credits_draw_lines (alpha kept in memory in the binary: else-branch, double / float divisors, pointer forms),
credits_parse_table (EBX / EBP / ESI roles of out / start / end), localize_set_string (dest in a stack slot, id in
EBX), key_clear (all 48 store orders), gr_project_vertex (the binary reloads flags for the final return; only a
volatile read reproduces it), gr_cull_bounding_box_transformed (no plane constructor in the binary under /Ob0),
avoid_spline_point_create (24 orders of the four stores), avoid_circle_contains_point (`fld st(0) / fmulp` for
r * r), avoid_sort_refs_along_segment (EBX / EBP), avoid_do_poly_seg_list_links (the door offset through one
vector2 temporary gets the frame right, 0x24, but p.y kept on the FPU across the branch and the node pointer bias
differ), seg2d_point_near_middle (operand order of the second dot product, register of s->a / s->b),
pf_first_edge_crossing (the binary computes dx * dx once from the register and once from memory: no CSE-proof form
found), LineFile::detect_eol (the binary tests `cr > cr + 1` before the lf test in the CRLF case: not plausible
source found).

**D11600 (vsdk/unnamed_gr_poly_0x5450c0.cpp; m-1020 worker 7). gr_poly_set_texture and gr_poly_rgb call float_max /
float_min out of line: `#pragma inline_depth(0)` around each of the two functions.** The binary calls 0x50d960 /
0x51faf0 from both (0x545394.., 0x545663..), while the rest of the unit expands its inlines (the unit under /Ob0
loses five matches). As D9810, the pragma sits above the address comment and is reset after the function. With it:
each clamp's inner call goes into a local first (`t = float_max(p, 0.0f); r = (ubyte)float_min(t, 255.0f)`, D6005),
the product goes through a float local (pushed as a dword); set_texture has one float local per channel (the third
lands in sum's slot, as the binary) and its w / h in the `bm > -1` block (frame 8, w in bm's dead argument slot);
gr_poly_rgb writes the pass record through a pointer with the cursor advanced after the three stores (found by
trying 144 statement orders; 4 match). Both exact; behaviour unchanged (the dropped red tint of gr_poly_rgb is the
binary's).

**D11601 (m-1020 worker 7). Source shapes that gave byte matches under 8966** (strict sweep, no match lost):
- A tail the binary duplicates is written twice: inventory_update_equip_slots tests equipment[8] in both branches of
  the equipment[7] test (orig 594 bytes, ours was 510).
- A range test laid out as the binary branches: monster_layout_load's delete test `type >= 4 && (type <= 5 ||
  type == 9)` (the binary's `jl` skips the == 9 test; same result) and its record loop as `while (1) { type =
  read_int(); if (type == 0) break; ... }` (D5904).
- Locals into dead argument slots (8966 does it by itself once the live range allows it): ix_sphere_polygon's te
  as a loop-body local (r's slot), ui_checkbox::create's width computed into a local before the height (label / font
  then stay in EDI / EBP and tw / th go to their slots).
- Declaration order of an initialised local against an object with a constructor: ix_ray_boundingbox's `int inside
  = 1;` before the profile_label (the binary sets EDI before the constructor call).
- The row clamp `if (count < page) rows = count; else rows = page;` from the globals, locals declared at the top
  (status_draw_skills, inventory_draw_list_rows; the coordinator's tip from another worker).
- The D10600 clamp `x -= page; x &= (x <= 0) - 1;` on the global: item_info_popup_page_up, status_page_up.
- inventory_preview_view_begin moves gr_screen.offset_x / _y in place (`+=`), not through x / y locals.
- ai_do_frame: `t1 = timer_get(..); ai_prof_do_frame_us += t1 - t0; ai_prof_path_us += t1 - t0;` (the binary
  subtracts twice).
- version_read_nfo_file's buffers are 513 bytes (frame 0x414: each rounded to 516); fgets reads at most 511.

**D11602 (vsdk/unnamed_ix_0x53ac20.cpp; m-1020 worker 7). BEHAVIOUR FIX: ix_point_in_polygon tests the first
triangle before the loop condition.** 0x53b791-0x53b7af: `mov ebp, 2` and the pu / pv set-up fall straight into the
loop body; the count is compared only at the bottom (0x53b8c2 `inc ebp; cmp ebp, eax; jge`). So the triangle
(verts[0], verts[1], verts[2]) is tested even when n <= 2. The source had `for (i = 2; i < n; i++)`; now a
do / while. Callers pass polygons of three or more vertices, so the result is the same in practice. The function
still differs (the binary keeps alpha on the FPU stack for the second test: `fld beta; fadd st, st(1)`).

**D11603 (notes only; m-1020 worker 7). Kept, closer but still differing; and tried without result.**
Kept (same behaviour): inventory_popup_result (`case 4` with its own body gives the 5-entry table 0x416bfc; key /
zero register swap left), inventory_use_selected_item (the equipped branch as if / else chains, 404 of 432 bytes;
8966 merges the two identical begin blocks and the list branch's flag tests into `test 6`), editor_dialog_handle_event
(the not-a-button case and the not-pressed case leave the loop so that one `mov al, bl` exit serves all: right size;
the event load is hoisted above the compares), ai_state::call_for_help (the team / type test as `(a == b) & (c == d)`,
the binary's sete / sete / test; one register too many), pick_attack_slot (`dir = him - me`: the dot product is
right; get_center's x operands stay swapped for every form, as w6 found).
Tried: inventory_destroy_selected_item / identify_item / equip_item clamp after min (25 forms: the AND lands in the
right direction only with the value in ECX, the binary has it in EAX), ai_state::load (register rotation of the goal
copies), wav_open (nBlockAlign operand order: no form moves it), cast_effect_track_add (index / pointer registers),
version_check_run (zero / three constant registers and merged tails), ui_radio_button::create (the width local makes
8966 keep font in EBP instead of parent), seg2d_intersect (120 declaration orders: dx1 / dy1 never reach the a / d.y
slots), ix_linesegment_sphere (nested return-false forms reach the right size order but len stays out of a's slot),
inventory_draw_list_rows-style shrink-wrapping elsewhere, ai_update_perception (8966 merges the party case into the
frame-time tail, the binary merges the idle-mode case), ui_checkbox::draw (FPU / integer scheduling).

**D11700 (m-1020 worker 8; game/new_journal.cpp, game/boss_ai.cpp, game/living_entity.cpp,
vsdk/character_instance.cpp, game/summoner.cpp). Source shapes that gave byte matches under cl 8966** (strict,
no match lost):
- journal_desc_page_up (0x41b570): D10600's clamp on the global, `journal_desc_first_line -= page;
  journal_desc_first_line &= (journal_desc_first_line <= 0) - 1;` (the seven forms of D10808 used a local).
- rider_unlink_mount (0x4a33b0): store order mount_flags, mount_link, ai.target, flags, obj_flags (found by
  trying all 120 orders of the five mount statements; two match).
- boss_mount_pick_flee_point (0x4a1ef0): the result through `out->set(p->x, 0.0f, p->y)` (the inline
  member loads z first and stores y = 0 between x and z; the vector3 constructor, member stores and a
  temporary do not).
- Character::get_resistance (0x4626b0): the non-magic tail as `int lv2 = info->resistance.level[type];
  r = resistance_level_damage_mult(lv2); return r + resist_equip_bonus(type);` (the register of the level
  load; either local alone does not give it).
- skeleton_get_bone_blend_weight (0x51a130): `time -= s->start;` instead of a `t` local: the binary keeps
  t in time's argument slot, fade_in in bone's (8966 reuses dead argument slots, D10402).
- character_instance::alloc_buffers (0x519570): the start-time loop without the `if (n > 0)` guard and
  through `times[i]` (no int pointer local): the binary's count / walk registers.
- summoner_cfg_load (0x43e250, closer, still differs): `int n = f.size(..); int size; if (n < 0x40) size =
  n; else size = 0x40;` gives the binary's `cmp eax, 0x40 / mov esi, eax`; the eax / ecx / edx rotation of
  the tail (sfx volume, music volume, the pxo flags) is left. All flag-update forms (separate &= / |=,
  or-then-and) compile to the same code.

**D11701 (m-1020 worker 8; vsdk/unnamed_vfx_0x536950.cpp). vfx_base::init_vis_flags (0x536ed0): the mesh flag
through the 16-bit view.** The binary sets and clears 0x400 with byte / word operations (`or byte ptr
[eax + 1], 4`, `and word ptr [ecx + 0x18], 0xfbff`): written through vfx_node_desc::vis_flags (the ushort of
the union at +0x18) instead of the int `flags`; the tag record through `vfx_base_tag *t = &tags[i]`, and the
reference indexed inside the loop (`e = &t->refs[j]`; the binary loads refs after the count test). Same
behaviour (0x400 is in the low word). Now exact. vfx_instance::process (0x536fb0, closer: 146 -> 28 differing
lines): the emitter search as `if (em != NULL) { while (1) { if (live) { ...; return true; } next; if (em ==
emitters) break; if (em == NULL) break; } }` gives the binary's block order (the found block inside the loop
body's jump target, the not-found block last); left is the eax / ecx / edx rotation of the later call
arguments.

**D11702 (m-1020 worker 8; vsdk/unnamed_snd_0x5204c0.cpp). snd_play_3d, snd_calc_volume_3d,
sound_emitter_calc_volume.** snd_play_3d (0x521480): the backend test in snd_play's shape, `int rval = -1;
switch (snd_backend) { case 1: if (ds3d) { ...; rval = ds_play_3d(..); } else { ...; rval = (looping ?
snd_play_looping : snd_play)(..); } break; } return rval;` (the binary's `or eax, -1 / dec ecx / jne` before
the epilogue; returns inside the case, a default or a plain if do not give it). snd_calc_volume_3d
(0x520fa0) and sound_emitter_calc_volume (0x5210b0): the `zero` that the binary keeps in a frame slot
(stored at entry, loaded when the divisor is 0; D10101 found no form) is an int read as a float, `int zero =
0; ... v = *(float *)&zero;` (the address-taken local stays in memory; `volatile` keeps it there too but pins
the store's position). A reproduction device, probably not the original's form; same value (+0.0f).
sound_emitter_calc_volume also needs `v = s->volume; v *= scale;` (the binary loads the volume first, as in
snd_calc_volume_3d). All three exact.

**D11703 (m-1020 worker 8; game/living_entity.cpp). /Od frames by the D6101 buckets, read off the binary with an
aligned listing, and behaviour-neutral shapes.** A helper aligns our listing with Sum.exe's (LCS over the
instructions with the ebp offsets masked) and pairs the ebp offsets of aligned instructions, giving each of
our locals its binary slot; the locals are then renamed so that their buckets are non-decreasing from the top
of the frame, and declared in reverse frame order (ties: the newest declared sits highest). Matches:
- Character::on_death (0x4537f0): locals renamed (q5, xp_total4, ratio_, n4, dist_, plz, c2m); the
  experience share clamp written as D10702's self copy (`if ((share = xp / n) < 2^31) share = share; else
  share = 2^31;`: the binary's fst / fcomp and `mov ecx, [share] / mov [share], ecx`); and the behaviour fix
  below.
- Character::monster_drop_loot (0x450e30): 14 locals renamed (the LOOT_UNIQUE macro follows the new names),
  and the three difficulty tests one `||` condition (the binary jumps to one shared `continue`).
- Character::find_ground (0x455fb0): the unused 0x2c-byte record at ebp-0x3c is a `lobj_hit` whose
  num_hits is cleared (it was an int; the frame is 0xf8 as in the binary), locals renamed, and the
  declarations ordered so that the ray, the list pointer and the first profile label are initialised in
  the binary's order (ray constructor, list = NULL, EH state 0).
- Character::init (0x44e1c0) is a byte match once charinfo_create_character_instance takes `int summon`
  (m-1020 worker 9's header change; the caller pushes the dword, the callee tests it with `test edi, edi` and
  forwards it unconverted); with the bool declaration of this copy only the `setne` conversion differs, so
  it stays likely. Shapes: the vfx mempool chosen through a local `pool` (if / else; the ?: temporary sat
  below `this`), the hero test a switch over the four hero species (the binary's switch temporary at
  ebp-0xe4 and its range test), the light through the inline attached_light_create_from_vfx (the binary's
  argument temporaries at ebp-0xd0..-0xdc), `unsigned int j` for the alternate-texture loop (`jae`),
  locals renamed.
BEHAVIOUR FIX: on_death returned when the current move's trigger 9 time left was above 0.0001; the binary
(0x4538b0: `fcomp 0.0001; test ah, 0x41; je 0x4538c2` to the fx_remove_missiles_from call, else `jmp
0x454060` to the return) returns when it is at most 0.0001, as play_move does (D6104). Now `<= 0.0001f`.
Not reproduced: the extra fst temporary of the inline vector3::len() in /Od (get_speed, anim_speed_at_blend,
stopping_distance, distance_to_reach_*): (float)(float), a (double) argument, a functional cast, sqrtf, a
nested inline with an argument, `return r` forms and len_sq() inside give no second store without an
argument store. ai_travel_offset: only the slots of the two inline temporaries (operator+'s pointer and the
copy target) are swapped.

**D11704 (notes only; m-1020 worker 8). Tried without a match.** matrix::determinant (8966 sorts the terms
and factors: 36 term orders and 6 factor orders give one code; an array view and a `float (*)[3]` view give
the binary's term order but not its factor order), matrix43::transpose_in_place (the binary stores a
temporary to memory four times; four-statement rotations keep it on the FPU), matrix43_interpolate_quat (the
`mov edi, eax` scheduled before the last faddp), boss_threat_add (24 store orders and chained assignment;
our eax copy is scheduled earlier), journal_handle_click (frame now 0x10 with dx / dy / dz at function
scope; the constant 2 is in EDI where the binary has EBP), vfx_instance::init_nodes (this in EDI, the binary
ESI), sum_root_velocity (the binary strength-reduces 4*i + 0x54 into EDI), the snd_set_volume family (eight
more clamp forms on top of D10101's 26), mp_handle_combat_sequence (int locals, a ubyte special, memcpy
reads), machival_ai_select_action (the binary keeps `target` in a frame slot and saves no EBP),
update_head_look and clip_move_to_navmesh (the binary frames hold unnamed vector / matrix43 locals and the
code differs as well). boss_ai_unload: every case tail-jumps with ECX unchanged, so the per-boss unload
functions are probably ai_state members (thiscall); not changed (their definitions are in other units).

**D11800 (include/engine/characterinfo/characterinfo.h, game/characterinfo.cpp). HEADER CHANGE:
charinfo_create_character_instance's `summon` is int, not bool.** 0x448de0 loads the argument whole into EDI (`mov edi,
[esp + 0x24]`), passes it unmasked to charinfo_select_mempool (int summon) and tests it with `test edi, edi`; a bool
parameter gives `and eax, 0xff` and a byte test. With int, 8966 also puts `start` (pool->offset) in the dead argument
slot as the binary does. The second difference was `unsigned int end = pool->offset;` read right after
set_current_mempool(NULL), before the summon / hero test (the binary reloads the offset there). Callers pass true /
false or living_entity's summon local: living_entity.cpp unchanged (111 matches before and after). Won 0x448de0.

**D11801 (include/vsdk/ca/character.h, vsdk/character.cpp). HEADER CHANGE: ca_preload_offsets and ca_preload_sizes
are one object, struct ca_preload_table ca_preload_tbl (0x2ee20c4, 0x230 bytes).** ca_load_preload_index (0x541790)
walks one pointer from &sizes[0] and stores offsets[i] at [esi - 0x118]: only members of one object have a known
distance. The old names are macros. Users: character.cpp only (ca_preload_find still matches). Won 0x541790.

**D11802 (m-1020 worker 9). Source shapes that gave byte matches under 8966.**
- str_hash_nocase_rol6: a local walking pointer `p` (the binary keeps the string in EDI, the hash in ESI).
- mp_charselect_update_create_button: `if (strlen) { if (species >= 0) &= ~4; else |= 4; } else |= 4;` (8966
  cross-jumps the two written-out `|=` tails the way the binary does; one `&&` condition does not).
- max(x, 0) into a global (D10600 extended): the min written as a ternary straight into the global, then
  `g &= (g < 0) - 1` (setl) or `(g <= 0) - 1` (setle), as the binary's test: mp_char_create, mp_charselect_init
  (also the selection store after the clamp), mp_levelselect_add_chat_line (`sets`: `< 0` on the difference).
- mp_charselect_init: update_scrollbar(0) in the list-0 branch and update_scrollbar(i) in the other, not one call
  after the if (the binary pushes 0 / i in each branch and shares the call).
- mp_levelselect_draw_levels: `rows = count < page ? count : page;` keeps rows in a frame slot as the binary.
- charinfo_remap_town_moves: the list-5 loop as `while (m) { ...; m = m->next; }` (the for form schedules the next
  load after the sound copy).
- charinfo_load_script_anim: `ci->times[..] = ci->character->skeletons[..]->start` (the binary reads the instance's
  character, equal to ca by the test).
- ui_listbox::create: store order flags, bm_background, max_content_w, max_content_h (24 orders tried).
- ui_listbox::scroll_page_down: the BY_ENTRY | ALIGN_BOTTOM tail `vscroll = e->height + e->y - view_h` written in both
  branches (the binary has two copies).
- cmd_tentacles / cmd_tentacle_test: `c = living_from_handle(..); c->flags |= CF_HIDDEN;` (the constant 8 then stays
  in EBP as in the binary) and `c->pos -= vector3(2, 2, 2)` for Flece.
- movement_read_controls: the unit is /Ob0 (vc6-flags.txt; the binary calls vector3's default / copy constructors,
  scale, scaled, assign and add_assign out of line; the other three functions match either way). Locals declared
  where first used (vector3 max_vel = ..., vector3 v, vector3 delta), the key differences through float locals
  (`d = kdt(a) - kdt(b); v.x = d * max_vel.x;`, `t` for the turn targets).

**D11803 (m-1020 worker 9; closer, still differing, kept).** nw_NewSendReliable: `send_header.seq =
rsocket->ssequence[i] = rsocket->oursequence;` (one load as the binary; a push scheduled late remains).
nw_ConnectToServer: the new link addressed as nw_reliable_sockets[i] (the binary uses the byte offset, no pointer
local; first difference moved from the loop to the server port load). tentacle_beast_boss_load: `ca_character *ca`
for the four find_mesh_tag calls, `t->reach = (owner->pos - t->swipe_pos).len()`, `squeeze_pos += owner->pos`
(only the last find_move's push order left). tentacle_beast_ai_frame: the start_grab test after the result switch
(`if (result == 3 && move == tentacle_move_86)`, the binary's default case jumps to it) and `owner->combat.target =
this->target = ...`; the code is identical now, only the function's exit block sits after the stance block in the
binary (8966 puts it at the end; early-return / else forms tried).

**D11804 (m-1020 worker 9; notes only). Tried without result:** charinfo anim-size sum in alloc_level_buffers (48
statement / parenthesis forms and extern declaration orders: the load order does not change; strict `target` stays),
report_memory (18 term orders, unsigned constants, shifts), mp_charselect_page_down and both drag_scrollbar functions
(list / index register swap), item_page_out_all, ui_gadget::send_event / set_parent, nw_accept_reliable,
nw_buffer_pop (struct copy forms), nw_NewCloseSocket (pointer and inverted-test forms), ui_listbox::get_vscroll_fraction,
set_vscroll_fraction (loop forms), scroll_page_up, ui_process (chained false initialisers), ca_character::add_skeleton
(exit block placement, as ai_frame), the row clamps in the two handle_click functions (frames differ first).

**D11900 (game/unnamed_interface_0x411270.cpp; m-1020 worker 10). Four interface helpers, shapes under 8966.**
party_find_next_member / party_find_prev_member (0x413200 / 0x413290): the NULL test and the handle read go
through status_character itself (`if (!status_character) return NULL; p = player_from_handle(status_character->
handle);`), with `c` copied from it first: the binary loads the global into EAX, copies it to EDI and reads the
handle through EAX. prev is written as next (start = p after the NULL test, the skip mask from two ifs); the
ternary mask gave neg / sbb / and. ui_draw_panel_frame (0x4134b0): the border step's offset `d = border - i` is a
block local, the height `h - d * 2` (8966 otherwise rewrites it as `h + (i - border) * 2`). ui_tooltip_set
(0x414750): the clamps at 0 are D10600's `x &= (x <= 0) - 1` on the stored globals, and the y sums subtract
pad_top before pad_bottom (strict mode showed the two loads swapped). All four exact; behaviour unchanged.

**D11901 (game/new_hud.cpp). hud_draw_tooltips: kind and id are local to the hot-button block.** The binary's
frame has kind at 0x0 and id at 0xc around w / h (0x4 / 0x8); with all four at function scope 8966 gives w, h, id,
kind for every declaration order (24 tried). Declared in the `if (hot_button)` block they take the binary's slots.
Exact. hud_load_bitmaps (still differs): the per-button filename is 128 bytes, which gives the binary's frame 0x8c
(the scheduling of the progress_update push before the six stores is left).

**D11902 (vsdk/unnamed_ds_0x55bf90.cpp). Lock / fill shapes and descriptor store order.** ds_fill_buffer (0x55cae0):
`if (buf->Lock(...) >= 0) { UINT got; ...; return r; } buf->Release(); return -1;` (the success branch first, got
local to it: 8966 then puts got in ck's dead argument slot as the binary does, frame 0). ds_stream_read_chunk
(0x55d530): the same branch order with the lock failure as an else that sets r = -1 and falls into the one
`return r`, and `UINT want = acm_stream_size(s, n1);` before wav_read (the binary calls it before loading the
other arguments). ds_create_buffer (0x55ca10): the DSBUFFERDESC stores in the order dwSize, dwFlags,
dwBufferBytes, lpwfxFormat (found by trying all 24). All three exact.

**D11903 (include/vsdk/snd/ds.h, include/vsdk/snd/snd.h, vsdk/unnamed_ds_0x55bf90.cpp, vsdk/unnamed_snd_0x5204c0.cpp).
HEADER CHANGE: ds_load_buffer's and snd_load's streamed parameter is bool.** ds_load_buffer passes it to
ds_load_buffer_data, whose second parameter is bool (exact), with `mov eax, [esp + 0x2c]; push eax` and no `test /
setne` (0x55c890); an int parameter makes 8966 emit setne. Its only caller, snd_load (0x5209c0), pushes its own
argument the same way (`mov eax, [esp + 0x438]; push eax` at 0x520b57), so snd_load's parameter is bool too
(otherwise the int -> bool conversion appears there). Every snd_load caller passes the constant 0 or 1 (boss_ai,
characterinfo, gamesound, level_script_wolong_caverns, unnamed_snd), which compiles the same for both types.
ds_load_buffer also increments the reference count of the new buffer through `ds_buffers[i].ref_count++` (not
b->), which VC6 cross-jumps with the already-loaded branch as the binary does. ds_load_buffer exact; snd_load still
exact (the only change in unnamed_snd_0x5204c0.cpp is its definition's parameter type). Whole-tree strict sweep: 0
lost; mingw: all 259 .cpp, only unnamed_decals_0x4bbe80.cpp's 9 known errors.

**D11904 (vsdk/unnamed_inetgetfile_0x51b760.cpp). InetGetFile::InetGetFile's directory buffer is 256 bytes, not
MAX_PATH.** The frame is 0x104 (`sub esp, 0x104` at 0x51b791): the new-expression temporary at +0 and the buffer at
+4 (`lea edx, [esp + 0x14]` after four pushes). With MAX_PATH (260) the frame was 0x108, the only difference.
Exact. A local file name of 256 characters or more overflows the buffer in the original (no caller passes one).

**D11905 (game/unnamed_tiger_0x499e50.cpp). ai_state::tiger_wander_update: `t->centre_offset + t->pos`.** The
binary loads pos first (fld [esi+0x10] / fadd [esi+0x28]); 8966 loads the inline operator+'s right operand first.
Same sum. Exact.

**D11906 (game/level_script_catacombs.cpp). ls_catacombs_use: one ball 1 block before the final return.** The
binary sends Puzzle-Ball01 and Puzzle-Pedestal01 to one lsf_ball1_used call (0x4fe8dc) that falls into the shared
`xor al, al` return (0x4fe8e1), while the ball 2 calls stay inline. `goto ball1;` from both tests with
`if (0) { ball1: lsf_ball1_used(); } return 0;` at the end gives exactly that; a plain label after `return 0` (or a
`goto done`) makes 8966 merge the ball 2 blocks as well. Same behaviour; exact. The form is a reproduction device.

**D11907 (game/unnamed_states_0x431870.cpp). loadgame_fill_save_list takes the list entry.** The inline helper
reads `e->text` itself (`level_script_find(e->text)`); with the name passed in, 8966 assigns the registers of its
loop differently in both expansions. state_loadgame_enter (0x431e80) also resets the list head with `list.prev =
list.next = &list` (next stored first). state_loadgame_enter and loadgame_level_list_cb (0x432200) exact.

**D11908 (game/new_hud.cpp). hud_confirm_level_exit: the multiplayer "Exit to %s?" format string is read into a
local before level_table_display_name** (D6205 / D10401: the binary loads it into ESI before the call). Exact.

**D11909 (game/new_automap.cpp). automap_drag_scroll: the binary's min / max shapes.** The min result is a
separate local (`m = min(x, half)`: the binary keeps x in ECX and the result in ESI), the lower bound of x keeps the
bound (`lim = ...; if (m > lim) lim = m; scroll_x = lim;`) and that of y keeps the value (`if (m <= lim) m = lim;
scroll_y = m;`), and the sums start at the drag start (`start_scroll - start_mouse + game_mouse`: strict mode showed
the scroll and mouse loads swapped with the other order). Same values. Exact.

**D11910 (notes only; m-1020 worker 10). Tried without a match.** state_title_enter (the flag merge: seven forms,
all compile to the same code), ui_choice_popup_show (the width maximum: 14 forms), ui_message_box_show (the binary
tests both flags against a shared `mov al, 1`; nested ifs, byte casts, a constant local), interface_init_global
(the binary keeps base_w in memory and screen_w on the FPU; switch / if combinations), ui_3d_begin (now `if (light)
{ track = 0; do { ...; track += 0x70; ... } while (light); }`, which fixes the counter's init and increment; the
load of base->light_tracks is still scheduled earlier than in the binary), hud_load_bitmaps (push scheduling),
hud_portrait_get_color (the binary cross-jumps the colour tails of cases 2, 4 and 5; clamp forms), hud_interact_
nearest, tiger_anim_remap_in (the zero in EBP and ready in c's slot, as serpent_anim_remap_in), tiger_path_to_target
(the binary calls the vector3 constructors out of line, frame 0x54), polygon_clip_to_plane (branch shapes compile
to the same code), ds_stream_open (store and return shapes), chat_input_handle_key (case order and fall-through
forms), chat_draw_input (the binary pushes the saved registers before the strcpy), projector_record::to_lobj_space
(the binary calls the vector3 copy constructor 0x410cd0 and plane::operator= 0x4c0a20 out of line).

**D12000 (vsdk/plainstring.cpp; m-1020 worker 11). int_to_string_with_commas: `first` is the loop index.** The
binary keeps the first group's length in EBX for both the strncpy count and the copy position, and turns the group
loop into a down-counter over a pointer into buf. `while (first < len) { dest[pos++] = ','; strncpy(dest + pos,
buf + first, 3); first += 3; pos += 3; }` gives it (the increment of first before pos fixes the last two adds;
a `for (i = first; ...)` index or a pointer walk does not). Same behaviour; now exact.

**D12001 (vsdk/unnamed_ftp_0x559ea0.cpp). ftp_get::ftp_get: the FreeSpace 2 shapes.** The user / password defaults
are an if / else of two strcpy calls (`if (u) strcpy(user, u); else strcpy(user, "anonymous");`, likewise the
password): the select-then-copy form kept 0 in EBX and compared INVALID_SOCKET with an immediate. In the slash scan
both pointers are set from `p + i + 1` before the filename copy, and `last` is cleared before `first` (the two XORs).
Same behaviour; now exact.

**D12002 (game/new_gamelist.cpp). gamelist_page_up clamps the global in place.** `gamelist_scroll_pos -= rows;
gamelist_scroll_pos &= (gamelist_scroll_pos <= 0) - 1;` gives the binary's `and eax, edx` (the max(x, 0) clamp of
D10311 / the round's tip). A local plus the ternary, the mask on a local, `if (pos < 0)` all leave `and edx, eax`.
Now exact. (The pickup unit's page_up, noted by worker 10, has the same shape.)

**D12003 (game/levelrender.cpp). levelrender_sum_point_lights: the distance sums z*z + x*x first.** vector3::len
gives z, y, x; the binary adds z*z + x*x, then y*y. Of the 12 orders and bracketings tried, `(float)sqrt(d.y * d.y +
(d.x * d.x + d.z * d.z))` (or with z, x inside) gives it. The float sum is the binary's. Now exact.

**D12004 (game/spells.cpp). spell_place_ground_effect fills the avoidance record in the binary's store order**
cx cz ax bx shape extent_a _unk00 az bz extent_b _unk28 owner (the scheduling the match27 note left open: the order
is the source order). Now exact.

**D12005 (game/unnamed_dialogue_box_0x4d6660.cpp). dialogue_topic_list::add: one layout() and return after an if /
else.** The ring-full branch and the append branch end in a shared `layout(); return e;`; VC6 duplicates the tail as
the binary has it, and the argument loads of the append branch's init come out in the binary's order (D7702's
"scheduling"). Now exact.

**D12006 (game/spells.cpp). spell_spawn_creature: each count cleared after its list head; n_others decremented after
the append.** `friends` head, `n_friends = 0`, `others` head, `n_others = 0` (the binary's store order), and in the
pick of a new "other" `n_others--` follows live_list_append as `n_friends--` does in the friend branch. Same
behaviour; now exact.

**D12007 (game/unnamed_dialogue_box_0x4d6660.cpp). dialogue_box::scroll_next_page: the lower bound in a local.** The
binary calls gr_get_font_height twice (the max macro would evaluate its second argument twice, three calls):
`int lim = gr_get_font_height(-1) * lines - text_height; scroll_target = max(scroll_target, lim);`. Same values
(the font height does not change between the calls); now exact.

**D12008 (m-1020 worker 11; closer, still differing, behaviour identical).**
- dialogue_line::draw (0x4d6b80): `if (kind == 0) {...} else { switch (kind) { case 1: red; case 2: gray; default:
  return; } one loop }`, the quote colour as an if / else of two calls, the line's y (`y + scroll + top`) in a local
  before the segment loop. The frame is now 0x848 with `this` in a slot, as the binary; left: 8966 associates
  `y + scroll + top` and `i * font_h + scroll + top` differently from the binary in every order tried (14 forms).
- dialogue_topic_entry::draw (0x4d6e70): the gray / yellow colour as an if / else of two calls (the binary's
  push / jmp / push / call); left: the same association of `y + top + scroll` and argument scheduling.
- File::read (0x525fe0): the binary copies len into a frame slot at entry and keeps len - n in EDI, reusing len's
  slot for the refill size and min_ver's for dst: `int total = len;` after the first copy, `len -= n`, `total != n`
  at the end. Left: the entry copy is made later than the binary's.
- ai_state::wall_avoid_steer (0x4b2890): the centre through `center.set(pos.x + off.x, ...)` evaluates z, y, x as
  the binary (get_center and operator+ evaluate x first); left: the fadd operand order and the `clear` block
  placement (the binary pushes EDI after the first test).

**D12009 (m-1020 worker 11; notes only). Tried, not reproduced.** effects_parse_arr_line (the entry test of the name
loop jumps to the other `return -1`; 9 loop / return shapes), ls_ikaemostopint_init (the constant 1 in EBX; chained
and int stores), dialogue_box::last_visible_line / draw and dialogue_topic_list::draw (EBX / EBP / EDI choice for
`first`; locals, operand orders and renames do not move it), character_use_inventory_item (register choice; ce / type
locals), spell_roll_amount (the binary's `dec eax; lea esi, [eax + 1]` trip count after a `count > 0` test: 12 loop
forms), steer_jitter (the two vector temporaries' slots), spell_fire_missile (h in one register for both creates),
effect_base_read (push esi before the bound test), rfx_preload (`buf_size + offset` loads offset first; declaration
orders), vfx_load (signed bit-field copy of the base flags), ui_button::process (shrink-wrapped push ebp),
lobj_ray_test_faces (mixed operand order of the inline dot; 48 hand-written orders all give one canonical order),
lobj_raycast (the end - pos temporary copy; 6 forms), spell_target_value (which identical return block survives
cross-jumping), cast_spell_update (the duplicated fail tail is merged), spells_parse_table (ESI / EDI swap).

**D12200 (vsdk/unnamed_vfile_0x5128b0.cpp, vc6-flags.txt). The vfile unit is compiled /Ox /Ob0 /GX /MT /Ow.**
File::calc_crc32 (0x514d60) calls Crc32's inline constructor out of line (its COMDAT 0x514eb0 sits right after the
unit's code, and plainstring.cpp starts after it), which VC6 does only without inline expansion; parse.cpp, which
calls the same COMDAT from TextFile::Checksum, is already /Ob0. Under /Ob0 the unit keeps all 77 matches of /Ox
(checked address by address) and calc_crc32 matches as well (D7502 rule: the new set contains the old).

**D12201 (worker 13, m-1020). Source shapes that matched under 8966.**
- A loop over a global table written with a signed index (`for (i = 1; i < FILE_MAX_PATHS; i++) ...
  file_path_table[i]`) instead of a walking pointer: VC6 strength-reduces it to a pointer but keeps the signed
  `jl` (file_add_path 0x512e60; File::find 0x5141c0; file_list_files gets closer).
- A loop that the binary does not rotate (`cmp / jge exit / jmp top`, or the end test first in the body) is
  `while (1)` with the test as an `if (...) break;` (FileReader::read_string 0x513e00: end test at the top;
  ai_state::goal_find 0x475eb0: count test at the end, the skips `goto next` to it). `for (;;)` does not give it.
- List splices through a saved neighbour (`prev = p->prev; p->prev = node; node->prev = prev; prev->next = node`),
  and each sorted-insert loop starting again from `list->next` (file_find_files 0x5133b0).
- A chained field argument through a local (`const char *text = e->topic->text;`) gives the binary's register
  reuse (conversation::farewell 0x4d8760).
- The origin added to the argument itself (`pos += s->pos; target = pos;`): the binary loads pos first in both
  cases (File::seek 0x514910).
- File::find: the path_root flag as two find_in_path calls (if / else), `find_at(...) ? true : false`
  (test / setne), and the last loop's fall-through straight to `path_id = 0` (written with two gotos; VC6 does
  not thread the plain form).
- Locals of one sub-block declared inside it share the dead slots (cp / cb / ch of mp_handle_obj_update 0x4806a0,
  frame 0x28 as the binary); the opposite for mp_handle_new_path: `vector3 v` at function scope gives the binary's
  frame 0x30 (still differs: the vector2 argument is stored to the stack).
- `pickup_scroll_pos -= size; pickup_scroll_pos &= (pickup_scroll_pos <= 0) - 1;` for max(x, 0) on the global
  (pickup_page_up 0x420d20; coordinator's technique).

**D12202 (worker 13). Float constants corrected to the binary's (1 ulp).** mp_handle_obj_update: the heading
factor is 0x38c910a4 (`9.5875264e-5f`, the fmul at 0x480748 reads 0x581a7c; the source had 9.587526e-5f =
0x38c910a3) and the 10-degree threshold is 0x3e32b8c3 (`0.17453294f`, fcomp at 0x480814 reads 0x581a44; the
source had 0.17453292f). Found by the strict data-target check once the bytes matched.

**D12203 (notes only, worker 13). Tried, not reproduced.** Crc32::update (12 loop / operand forms and flags: the
result lands in EDX not EAX), StringPool::Add (s in EBP vs ESI, 13 forms), set_cipher_key (the binary's
load / and / store of the slot flag looks like a bitfield; would need a header change) and get_async_status
(`add eax, file_slots` then +0x8030: not reproduced by pointer locals), mp_handle_status_add (call hoisted gives
ndiff 2; the value spill slot / c reload order remains; 24 load orders identical), mp_handle_ai_disable (from in
ECX vs EAX), mp_send_new_lep (the binary keeps the send counter in ESI), mp_send_obj_update (qx / qy / qz slot
order; 24 declaration orders identical), pickup_draw_item_list (x / i register swap), ai_choose_special (the push
test block placed after the epilogue), mp_handle_it_okay, pathvis_bsp_contains (vector2 n gives the binary's
FPU sequence; the shared recursive-call tail is kept in case 1 in the binary, in case 2 by us),
do_single_vis_check (return-1 epilogues), matrix3_unpack_orientation, ca_render_morph_vertices (the anim test as
if / else halves the difference). Flags tried without gain: /Ox, /Ox /Ow, /O2 /Ob0, /O1 on evil_urath +
pathfinding_visibility; /O2, /Ox, /Oa on crc32 + stringpool.

**D12300 (m-1020 worker 14). Source shapes that made VC6 8966 emit the original code.** One exit / one result
variable: build_stamp_check 0x52f450 (`result = 3` before the test, every outcome assigned, one `return result`;
the binary loads 3 into EDX before the branch), savegame::data_offset 0x4cfbd0 (the second switch assigns and
breaks to the one return). Early returns instead of one guarded block: playmenu_frame 0x4217c0 (`if (input) return
key; if (box) return key;` keeps the store of the click result to the key slot). Common subexpressions written as
locals before the branch: savegame::set_flag 0x4cfd30 (`int byte = index >> 3; int mask = 1 << (index & 7);`
before `if (value)`), savegame::set_bit 0x4cff80 (byte then mask before the size check). The clamp
`x &= (x <= 0) - 1` on the global: saveload_page_up 0x426260. Case-local blocks: playmenu_open_save 0x421b00 (each
case's locals in its own block share offset 0; `save` is bool[4] as in open_saveload_screen, D7200).

**D12301 (unnamed_cutscene_camera_0x493e50.cpp). A parenthesised member operand in a float compare gives `fld m;
fld st(1); fcompp`.** cutscene_camera_sequence::sample 0x494310 clamps with `if (t <= (start))` / `if (t >= (end))`;
without the parentheses 8966 emits `fcom m`. Same family as the parenthesised float temporaries of D10402.

**D12302 (game/level_scripts.cpp). savegame::write_header copies its strings as memcpy(dst, src, strlen(src)).**
The binary's inline copies count `not ecx; dec ecx` (no NUL) into the fields it has just zeroed; strcpy would copy
the NUL too. level_time goes through `float t = (level_time);` (the binary's dead local). Behaviour is the same
except for a name that fills its field exactly (no NUL then, in the original too). Still differs in two registers.

**D12303 (m-1020 worker 14; notes only). Tried, not reproduced.** Base / index order of `[reg+reg]` (hot_button_set
stores, read_track keys: `p[k]`, `k[p]`, `*(p + k)`, post-increment forms all give the same order); pure register
swaps (get_int, has_conversation_data, level_item_has, level_format_display_name, data registers of savegame::read,
the flag remap of mp_player_file_read_summary); spill-slot order (write_item_objects count / float temporary,
layout_conversation's two pointer spills, camera_blend_sequence::advance's six angles: declaration order and names
make no difference, float[2] arrays shrink the frame); saveload_draw_info's small locals; gr_classify_sphere_view
(nested positive tests give the shared `return -1`, but 8966 keeps nz in memory where the binary keeps it on the
FPU stack); write_living's constant 4 cached in EDI; ls_sewer_create_avoid_spline (the binary has no vector2
constructor loop for avoid_spline_def's points: in the original the point type has no constructor; a header change
touching other workers' files, not made).

**D12400 (m-1020 worker 15: game/player.cpp, game/level_script_slums.cpp). Branch shapes that give build 8966's
code** (each a strict byte match): Player::is_leader (0x4339e0) is the if-form bool `if (flags & PF_LEADER) return
true; return false;` (the dword load, then `shr al, 3 / and al, 1`; `!= 0`, ternaries, shifts and bit-field forms give
`shr eax` / a byte load). Player::create (0x433790): the group test is nested, `if (game_flags & GF_SINGLE_PLAYER) {
if (leader != true) ai_group_add_member(..); else ai_group_create(c); } else ai_group_create(c);` (the binary pushes c
before the leader test and enters the create call after the push). ls_lenele1b_init (0x4fad90): the stage tests are
one `if (>= 6) .. else if (>= 4) .. else ..` chain with the v31-v33 calls written in both lower arms (one shared
`add esp, 0x24`; the nested form with the calls after the inner if pops per arm). Behaviour unchanged.

**D12401 (game/unnamed_camera_0x4778b0.cpp). camera::update_eye_position (0x478130): `float h = zoom * elevation;`
before the ratio, fabsf for the input yaw, and the constant 2.2573366f. DATA FIX.** With the product written inline
8966 loads elevation first (`fld [esi+0x188]`); through the local h (or `r = zoom * elevation + d.y` first) it loads
zoom first as the binary does. The turning test compares against the float 0.0 (`fcomp dword [0x57eb84]`): fabsf, not
fabs. The steep-view factor at 0x581a4c is 0x40107834 = 2.2573366 = 1 / (1.3 - 0.857); the source had 2.25734f, which
rounds to 0x40107842 (strict mode reported the function as `target`). Now exact. The old constant changed k by about
1e-6 relative in the rebuilt exe; the original binary is not affected.

**D12402 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_door_sound_radio_cb (0x4eb9e0): each radio branch
has its own found test** (`e = ..get_selected(); if (e != NULL) { set_text(e->text); return true; }` inside the
branch) and only the not-found `set_text("")` is shared after the if / else if. Now exact.

**D12403 (game/unnamed_script_editor_0x4de0b0.cpp, notes). script_create (0x4e88a0) writes the new action through
`script_editor_cur_script->actions[n]` each time** (the binary reloads the global after every store; the `a` pointer
local was 128 bytes short), with the Die case first in the switch (the binary's layout order; the byte index table
0x4e8cd0 sends 8 / 0x1d / 0x1f to the Play action case's lists_dirty store). Same behaviour. Still differs: 8966 keeps
1 and NULL in registers and n in a frame slot (frame 0x54), the binary keeps n in EBX, type in EBP, pushes ESI / EDI
after the 54-action test and stores immediates (frame 0x50: tmp 0x18 + the file list 0x38).

**D12404 (game/gametarget.cpp, notes). pick_test_character (0x43c620): `vector3 off, closest, d` at function scope and
`d = off - closest;`** give the binary's 0x34 frame (off +0x10, closest +0x1c, d +0x28 with d.y stored at +0x2c, d.x /
d.z on the FPU; the three float differences gave 0x28), every offset component stored in each branch (block-scope off
let 8966 carry a component on the FPU across the join), and the character branch's stores after the height call
(`off.y = ..; off.z = 0; off.x = 0;`; with x, y, z 8966 cross-jumps the z store into the container branch). 97 -> 40
differing lines; still differs in `dz2 + dx2 < r`, where the binary keeps dx2 on the FPU (`fld dz2 / fadd st, st(1)`,
popped after the tests) and 8966 adds dz2 into it. Behaviour unchanged.

**D12405 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_frame (0x4e01f0): `vector3 size, p` at function
scope, assigned through temporaries.** The binary's 0x24 frame is size (+0), p (+0xc) and one shared temporary slot
(+0x18) used by every vector expression (`size = l->bbox_max - l->bbox_min`, `p = pos + uvec * (size.y * 0.5f)`:
temporary plus integer copy, D10502). Block-scope vectors put the temporaries in other slots (the click-object loop's
at +0xc) and the hover p's product in the other operand order. Each object block computes the box size once and passes
it to both draws (the binary's second call takes the same frame slot), and each time branch has its own gr_string call
(the binary merges only from the mode push on). The SE_DRAW_LOBJ_ARROW macro is gone. Now exact; same behaviour (the
bounding box does not change between the two draws).

**D12406 (game/unnamed_evil_joseph_0x4c3470.cpp). Evil Joseph's AI: branch shapes.** ai_state::evil_joseph_ai_frame
(0x4c3860): `if (target != NULL && (target->flags & CF_HIDDEN)) target = NULL; if (target == NULL && submode !=
AIS_FOLLOW_PATH) { go_idle(); return; }` (the binary's null test jumps straight to the sub-mode compare and the hidden
case clears EDI first) and the teleport navpoint indexed in the path_find_new call (`&evil_joseph_teleports[i]->pos`,
`..->layer`: owner is loaded first). evil_joseph_ai_select_action (0x4c39b0): the first target pick is `if (handle != -1)
{ living_from_handle(..); set_attack_timer(..); } else return 0;` (the binary sends both pick failures to one `xor eax,
eax` tail after the switch). Both exact. evil_joseph_pick_helper_summon (0x4c3cd0, closer, still differs): the midpoint
is `evil_joseph_summon_pos = (boss->pos + t->pos) * 0.5f;` and the scattered point `dir *= r; dir += boss->pos;` with
dir declared in the scatter block, so the vector temporaries share its slot (frame 0xc as the binary); the FPU order of
the length (`dx * dx + dz * dz`) and of the scaled direction still differ. Same behaviour (float addition commutes).

**D12407 (include/engine/camera/camera.h, game/unnamed_camera_0x4778b0.cpp). HEADER CHANGE: camera::find_occluders
returns bool.** The cutscene early return at 0x478546 is `xor al, al` (with EAX already 0 from the index store before
it): only AL is written, which VC6 does for a bool result; an int result gives no instruction there (EAX is already 0).
With bool the function compiles the same `xor eax, eax` / `mov eax, 1` final returns as the binary (`return 1; return
0;` converted). The only caller (camera::frame, 0x478990, same file) ignores the result; its code does not change. The
scalar ray fields are now set before start / dir (first difference moves from +0x24 to +0x58); still differs in the
store schedule: the binary stores min_radius (+0x30) as an immediate after the start copy, ours stores it from the
lobj_ray constructor first (the constructor is in levelrender.h, shared). Same behaviour.

**D12408 (game/unnamed_camera_0x4778b0.cpp). camera::frame (0x478990): cam_vec assigns from a vector3 without a
temporary, p is constructed first, the min is an if.** With only the implicit cam_vec assignment, `last_target_pos =
target->pos` converts target->pos to a cam_vec temporary (an out-of-line vector3 copy constructor, an EH state per
temporary, frame 0x28); the binary copies three dwords and its frame is 0x10 (the min temporary and p). cam_vec now has
`void operator=(const vector3 &v)` (three stores) and an empty default constructor, and p is declared after the two
statics (the binary sets the EH state to 0 right after the static initialisation, before the target test) and assigned
from last_target_pos after the occluder counts are cleared. The height is `float r = target->bound_radius * 0.8f; float
h = look_height; if (h < r) r = h;` (the binary keeps r on the FPU and h in a frame slot; the min() macro reloads
look_height from the object). Now exact; same behaviour. The class is local to the unit.

**D12409 (notes only; m-1020 worker 15). Tried without a match:** camera::update_orientation (0x477f60): the ring
index wrap `if (--i < 0) i = 3;` compiles to `dec / jns` under 8966, the binary has `dec / test / jge`; `i--; if (i <
0)`, `(i -= 1) < 0`, `!(i >= 0)`, `0 > --i`, a ternary, `i = i - 1` and an if / else all give `jns`, `== -1` and `<=
-1` give `cmp reg, -1`. camera::auto_view_update (the shrink-wrapped `push ebx`, block-scoped near_enemy in two forms).
camera::set_mode (8966 keeps mode in ECX across the first test; four forms). spline::eval (0x558f40): 8966 folds
`-0.5f * u3 + u2` into `u2 - 0.5f * u3` sharing 0.5f * u3 with b3; 18 term forms and index / pointer forms tried; /Op
for the vsdk units loses matches (bezier 14 -> 10, anim_track 12 -> 8, skeleton 26 -> 24). character_can_talk_to's
length (`fld st(0) / fmul st, st(1)` for d.z; locals, sqrtf, operand orders). irc_process_input (the binary shares
the "xyz" sprintf with "list"; or / else-if / pointer forms). script_editor_door_sound_inputbox_cb (case order, if /
else). script_editor_focus_selection and world_pick_under_mouse (the binary expands only the first inline members and
calls 0x410cd0 / 0x413190 / 0x43b580 / 0x43b5a0 for the rest, D10808). script_write_objects (three buffers args[0x100],
msg[0x104], name[0x104] from the frame offsets: frame right, the register choice for c / level / the 7 constant still
differs; not applied).

**D12410 (m-1020 worker 15; game/multi_chattracker.cpp, game/unnamed_script_editor_0x4de0b0.cpp). Success branch
first.** irc_read_line (0x4ac100): `if (recv(..) > 0) { ..the byte.. } else { irc_queue_command(IRC_CMD_DISCONNECTED,
..); return NULL; }` places the line-complete block before the disconnect block as the binary does (with the failure
first 8966 lays them out the other way round and adds an `xor eax, eax` to the loop exit). Now exact. Closer, still
differing: script_editor_pick_hover (0x4e2bb0) with radius block-scoped in the character case and face in the two
level-object cases (frame 0x4c as the binary; the binary then copies the size row with integer moves and stores the
radius once after both arms), script_editor_ray_hit_object (0x4e2f30) with `vector3 centre` at function scope and
assigned (`centre = *pos + ..`, temporary plus copy: 75 -> 36 differing lines; the rest is the matrix copy, of which
the binary expands only part, D10808).

**D11500 (m-1020 worker 6; 11 units). Source shapes that make cl 8966 emit Sum.exe's code** (each a strict byte match,
no match lost; notes in m-1020 w6-NOTES.md):
- Constant-first float compares (`0.8f < amt`, `0.0f > amt`, `15.0f > far`) give the binary's `fld const / fcomp var`,
  and a float local read once (`float f = fps;`) keeps the value on the FPU for both tests (levelrender_fog_enable).
- Bit tests on an int global written as `(bool)(~flags & 1) || (bool)((flags >> 3) & 1)` give the dword load, `mov cl,
  al / not cl / test cl, 1` and `shr eax, 3 / test al, 1` (level_set_current_layer); `!(bool)bitfield` gives
  `shr edx, 2 / test dl, 1` for a bit-field member (dialogue_draw, dialogue_frame's flag2 / flag6).
- A min clamp of a parameter into a global is the ternary `g = p < 4 ? p : 4` (mp_host_game: the if form schedules a
  pop between the store and the test).
- A global store before an object's construction is written before the declaration: `num_detail_levels = 0;
  TextFile tf;` (detail_levels_parse_table), `gr_light_desc light;` after the first two statements
  (levelrender_render_scene, closer).
- Loop shapes: `while (1) { if (recv(..) != SOCKET_ERROR) break; if (WSAGetLastError() != WSAEWOULDBLOCK) return NULL; }`
  (http_get::get_line; `for (;;)` differs); the light delete loop of levelrender_setup_level_lights is
  `if (levelrender_num_level_lights) { int *p = &handles[levelrender_num_level_lights]; i = levelrender_num_level_lights;
  count = 0; do light_delete(*--p); while (--i); }` (the global re-read gives the binary's EAX copy).
- Store order: fds.fd_array[0] before fd_count (http_get::connect_socket); the slot store before the free_slot clear
  and `n` declared before `best_d` (group_assign_formation: the zero register then also feeds best_d).
- `first = last = p + i + 1;` before strcpy gives the strcpy temporary its own frame slot (http_get::init).
- Vector forms: `ray.dir = eye - centre; ray.dir.len_sq()` keeps the three differences on the FPU and stores x / y with
  `fld st(n) / fstp` (lamp_glow_update_lobjs / _effects); `p = vector3(base->pos)` gives the dead y / z temporary of
  the binary; `vfx_instance *inst = fx->instance;` before effect_get_base gives `mov eax, [eax+0xa0] / mov ecx, [eax]`.
- In a /Ob0 unit, gr::Mode's inline conversion is a call: pass `gr_string_mode.value` (dialogue_draw; the binary loads
  the dword). Other /Ob0 game units calling gr_string(.., gr_string_mode) may gain the same way.
- Sibling blocks with their own `int w, h;` (each arrow block of dialogue_draw) give the binary's swapped slots; with
  function-scope w / h both calls use one layout.
- Case bodies in the binary's order (spells_handle_click: Esc / C, S, I, R; I cross-jumps into R's tail); an element
  pointer declared after the tooltip test (`ui_button_state *b = &spells_buttons[i]`).
- `(count + first - 1) % 64` operand order (dialogue_draw's text box; the topic box keeps first + count).

**D11501 (include/engine/rendering/levelrender.h). HEADER CHANGE: lobj_ray's inline constructor clears min_radius as a
float (`min_radius = 0.0f`, was `_unk30 = 0`).** The field is a union of int _unk30 and float min_radius (read with
fcomp, D57). In lamp_glow_update_lobjs (0x484412: `mov dword ptr [esp+0x5c], 0x0`) and lamp_glow_update_effects
(0x484688) the constructor's store is an immediate although a zero register (EBP / ESI) is live; an int 0 makes 8966
use the register, a float 0.0 store is an immediate. Same bits. Full strict sweep of the tree: no match lost, +1
(lamp_glow_update_lobjs). mingw over all 259 .cpp: only unnamed_decals_0x4bbe80.cpp's 9 known errors.

**D11502 (game/unnamed_render_scene_0x481680.cpp). BEHAVIOUR FIX (D5107 applied): lamp_glow_update_effects takes
the glow point from the vfx_base's pos, not from +0x48 of the node record.** 0x484660 calls effect_get_base and EAX
keeps the result; 0x484690-0x484696 read [eax+0x4c] / [eax+0x48] / [eax+0x50] (vfx_base::pos); the node record is
only tested for flag 0x40 (0x48467d). The source read rec->glow_pos (the alias in vfx.h that D5107 kept for this file,
which is +0x48 of the node descriptor, inside its frame BitArray). Now exact. The vfx.h alias is no longer used by
this file (left in place; another unit may be checked before removing it).

**D11503 (notes only; m-1020 worker 6). Closer, still differing; tried without a match.** levelrender_render_scene
(the first 146 instructions now match; the sky loop's register choice), dialogue_frame (all but the final mode update:
the binary loads key into EDI before the mode test and shares one epilogue; if / else, nested, goto, else-if and
comma forms tried), levelrender_effect_apply_vis (count locals read once fix the three flag loops; the tag offset
and the set pointer take each other's slots), lamp_glow_update_effects' variants, pick_attack_target (`t = NULL` after
the weapon test puts t in EAX; the binary keeps -1 in EBP and stores max_attackers twice), giant_salamanka_ai_select_action
(score stored before handle; count = 0 after the busy test moves `this` out of EBP), level_vis_reset (an index loop
gives the biased pointer but 8966 then puts the constant 1 in a register), effect_stream_start (a volatile store does
not stop 8966 from reusing the 1 of the return), goal_do_travel (the bool argument goes through the dead goal_flags
slot in the binary), character_use_skill_on (shrink-wrapped EBX push), gr_bitmap_3d_projected (width / height as FPU
values; frame 4 bytes larger in every form), add_effect_entry (48 dot orders with the depth in a local),
levelrender_add_effects (operator forms change the frame by 12 or 24, the binary is +24 over ours).


**D12100 (game/level_script_khosani_lab.cpp; m-1020 worker 12). The puzzle corners set out through a two-argument
setter.** The binary computes the centre's y first and keeps it in a dead argument slot (`fst [esp+0x10]`) while x
goes straight to out->x: the arguments of a call, evaluated right to left. A file-local `static inline void
v2_set(vector2 *v, float x, float y)` gives it (the original probably had a vector2 set member). Each step is
`vector2 t = s * *d; v2_set(out, out->x - t.x, out->y - t.y);` (the products formed first, as the binary), and s is
`s = 0.3f; if (!inner) s = 0.7f;` (`fld 0.3 / jne / fstp st(0) / fld 0.7`; the ternary and the if / else give `je /
fld / jmp / fld`). corner_nn and corner_np exact; corner_pp differs only in its first y sum (the binary loads t.y
and adds the centre; 8966 keeps the centre first for every operand order and operator form tried).

**D12101 (game/script_internal.cpp). script_action_wander reads the navpoint layer into a local before the call**
(`int layer = np->layer;`): the binary computes `this` (`lea ecx, [esi + 0x324]`) before `&pos`. Exact.
script_parse_navpoints: the name buffer is 80 bytes (frame 0x11c, the binary's; it was a guess of 64); the function
still differs in the ray block (the constructor's `_unk30 = 0` store gets a shared zero register in ours).

**D12102 (vsdk/gr_direct3d.cpp). d3d_enumerate_ddraw_devices: every failure is `goto fail`, one `return NULL` after
the device return.** The binary falls through the final `jg` into the one NULL epilogue that all four failures jump
to; `if (n <= 0) goto fail; return &dev[n - 1]; fail: return NULL;` (with `int i` at function scope for the goto)
gives it. Returning NULL directly at each test leaves the first failure with its own epilogue; nesting the body
under the enumerate test lets 8966 reorder the tail. Exact.

**D12103 (vc6-flags.txt, vsdk/gr.cpp). vsdk/gr.cpp is compiled /Ox /Ob0 /GX /MT /Ow (was /Ox /GX /MT /Ow).**
gr_rect and gr_bitmap_float construct their local gr_vertex arrays with `vector constructor iterator` (0x404880)
and gr__Vertex__Vertex (0x5081c0), and 0x5081c0 itself calls vector3() (0x505530) out of line: the unit had no
inline expansion. With /Ob0 no gr.cpp match is lost once gr_set_gamma writes the math.h inline out as
`(float)pow(i * (1.0f / 255.0f), gamma)` (the binary uses the _CIpow intrinsic directly). gr_rect then matches with
the array declared after the clipping (the iterator call follows the clip tests) and each corner written as
`vlist[k] = &verts[k]; sx; sy; sw; _unk1a` (one of 24 store orders matched). gr_bitmap_float declares its array after
gr_set_texture (the binary's order) and is closer; its corner arithmetic still schedules differently. D7502's rule
holds (no match lost; 55 -> 56).

**D12104 (vsdk/registry.cpp). os_registry_resolve_key: one return of root, two walking pointers.** The binary keeps
HKEY_CURRENT_USER in EBP and returns `mov eax, ebp` from every path: one `return root;` at the end with if / else
branches (three returns let 8966 fold the constant). Both copy loops walk pointers: `for (p = subkey, d = name; *p
&& *p != '\\'; p++, d++) *d = *p;` (8966 merges the two induction variables into the binary's (name - subkey) + p
addressing) and `d = out_path; for (p++; *p; p++) *d++ = *p;`. Same behaviour, the known overrun included. Exact.

**D12105 (game/level_scripts_common.cpp). level_script_restart's door loop goes through `door_info *d =
&doors[i]`** (the binary keeps the record pointer in EDI and the byte offset in EBX, so the false argument is an
immediate push). Exact. Element-pointer shapes also brought party_set_level to a one-instruction difference (the
skill index in a block-scope `int sk`, the +20 / +10 as an if / else of two adds; left: the base / index order of
one `mov dl, [eax + ecx + 0x93]`) and script_guard_sees_player closer (`vector3 dir = cur_character->pos -
guard->pos;` built in place, `dir.dot(guard->orient.fvec)` for the binary's z, y, x order; left: 8966 keeps a zero
register in EBX, the binary pushes and stores immediates).

**D12106 (vsdk/gr_direct3d.cpp). gr_d3d_init stores d3d_init_done through a volatile lvalue** (D11305, another
worker's technique): 8966 hoists the immediate `d3d_init_done = 0` above the hwnd store, the binary keeps the source
order; `*(volatile int *)&d3d_init_done = 0;` keeps it in place. Exact. gr_d3d_draw_primitive is closer with the
lock-flag choice written `else if (pos != 0) nooverwrite; else discard;` (8966 still preloads one value instead of
the binary's if / else with a jump).

**D12107 (m-1020 worker 12; notes only). Tried without a match.** ls_ionaext_frame's `sete al / test al, al` (ten
bool forms, local, inline helpers); level_trigger_point_left_of_line's 8 extra frame bytes (unused, dead inline and
block-scope locals, a point array, other return forms); worldmap_frame's `fstp st(3)` before the pops (six forms of
the distance argument); worldmap_marker_clamp_to_edge's resolution_index load above fcos (operand orders, double /
float cos); vfx_tex_layer::get_frame_bitmap's this / n register swap (all declaration orders, block scope, return
forms); ui_slider::create_scrollbar's this / listbox swap (`visible = f(); view = visible;` kept: fewer register
differences); eax_init's block order (if / else, gotos, flat returns: 8966 always falls into the success block);
gr_d3d_set_window's lea base / index order (four operand orders); gr_line's clip_top / clip_bottom integer loads
(all 24 declaration orders give the same code); mp_player_alloc's `&p->prev` (pointer and reference locals are
folded); multi_process_client_status's store schedule; automap_labels_parse_table's early epilogue placement (nested
and flat forms); gr_tmapper_multipass's +8 bias of the pass pointer. cmd_disp_rate is closer with the table indexed
directly (no `t` pointer) and the ON / OFF message as an if / else of two sprintf calls; left: EBP pushed in the
prologue and the stack pops after sprintf merged in the binary.

**D12108 (include/engine/rendering/levelrender.h). HEADER CHANGE: lobj_ray's constructor clears +0x30 as the float
min_radius (`min_radius = 0.0f`), not as the int _unk30.** Same bytes (both store 0). Evidence: script_guard_sees_player,
script_parse_navpoint and script_parse_navpoints store the constructor's 0 as an immediate (`mov dword ptr [esp+x],
0`) although the function keeps a zero register; 8966 stores an int constant 0 from a live zero register but a float
0.0 always as an immediate, and the field is read as a float (lobj_raycast_draw_lists' fcomp, decomp57). With the
float store script_guard_sees_player matches (with D12105) and the two navpoint parsers come closer. Users: 17 units
(gametarget, level_scripts, level_scripts_common, living_entity, objects, script_internal, spells, the camera,
ghost rider, goals / doors, lobj raycast, luminar, render scene, script editor, shadows and tentacle beast units);
whole-tree strict sweep: 5086 match, 0 lost against vc6-matches.txt; mingw: 259 .cpp, errors only in
unnamed_decals_0x4bbe80.cpp (9, as before).

**D12109 (game/script_internal.cpp). The navpoint parsers' ground ray.** script_parse_navpoint and
script_parse_navpoints set the ray up with vector copies (`ray.start = np->pos; ray.start.y += 2.0f;`, `ray.dir =
vector3(0.0f, -1000.0f, 0.0f);`: the binary reads all three position components through the &np->pos register),
clear the hit record just before lobj_raycast_all (memset after the ray fields: with it first, 8966 stores the
float zeros from the EAX the rep stosd left, the binary stores immediates), and take the level object into a local
before pick_layer (`l = lobj_from_index(hit.face); np->layer = l->pick_layer(&hit_pos, 1);`: the binary pushes the
face before the hit point arithmetic and pick_layer's arguments after the call). With D12108 and the 80-byte name
of D12101 both now exact. Same behaviour. The same ray set-up may help the other lobj_raycast_all callers (other
workers' units).

**D12500 (game/new_dialogue.cpp; m-1020b worker 1). dialogue_draw 0x409650 passes gr_string_mode itself.** Since
D11003 gr_string takes its gr::Mode by value, so the `gr_string_mode.value` arguments (D11500's /Ob0 note, written in a parallel worker
while the parameter was still the raw value) became calls of the out-of-line constructor gr::Mode(gr_mode) (0x506810),
and the match recorded in m-1020 w6 was lost in that merge. Passing the gr::Mode object copies the dword the way the
binary does (`mov ecx, [0x25d8df4]; push ecx`); the function is exact again. Same behaviour. No other unit uses
`gr_string_mode.value`.

**D12501 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_default_action_navpoint 0x4e8cf0 returns a
`found` pointer.** The binary zeroes ESI before the cutscene-button call (0x4e8cf6) and returns it only when the level
table is empty or the loop's entry test fails (0x4e8d56 `mov eax, esi`); the address &navpoints[navpoint_count - 1]
is computed once before the loop (0x4e8d28-0x4e8d2e) and returned from the loop and after it. `navpoint *found =
NULL;` at function scope, assigned inside the loop and returned (the type test still through the NULL np, read as
[0x28]) gives that shape: 37 -> 21 differing lines. Still differs: 8966 keeps found in ESI throughout, the binary
puts the address in EAX and leaves the NULL in ESI. Tried: break / return in the loop, no outer count test, found
initialised from np inside the branch, a separate return after the loop. Same behaviour (D8702's logic fix kept).

**D12502 (game/unnamed_script_editor_0x4de0b0.cpp). script_editor_add_navpoint 0x4e5b10: the type dispatch is a
switch.** After the limit switch the binary tests `cmp edi, 0 / jl` (0x4e5b67), `cmp edi, 1 / jle`, `cmp edi, 3 /
jne` and lays the ambient case out first: a second `switch (type)` with cases 0 / 1 and 3 (the old `if (type == 0
|| type == 1) ... else if (type == 3)` compiled to `je` tests). The first difference moves from +0x59 to +0x365.
Still differs there: in the level case the binary computes `ray.start + ray.dir * hit.t` with its two temporaries in
the other order of slots (scaled direction low, sum high) and loads the scaled component first; the same statement
in the ambient case matches. Swapping the operands compiles identically under 8966, and moving the ambient case first
in the source changes nothing. Same behaviour.

**D12503 (notes only; m-1020b worker 1). Tried without a match.** wav_open 0x5670a0 (only the imul operand order of
nBlockAlign: operand swap, int casts, a pointer local all compile the same), item_page_out_all (EBX / EBP swap of
slot and zero: early-return, nested, else, loop and pointer forms), pxochat_page_down (EDX / ECX roles: eight clamp
forms, a page local, `last -=`), pyrul_fire_attack (`#pragma inline_depth(0)` gives the binary's 0x9c frame and the
out-of-line vector3 constructors, but also calls the empty default constructors and the inline operators the binary
expands: VC6's per-function inline budget, not reproducible by the pragma), handle_arrow_key (sw / sh / bottom and gw
/ gh slot order: declaration orders and a bottom without a local do not move them; distance sums are normalised),
level_item_ok (e / count in each other's dead argument slots; a restart loop with goto or an outer for brings the
walking name pointer, 168 -> 142 differing lines, not applied), titus_ai_select_action (goto / merged-tail forms of the
stage-1 retarget are worse). summoner_cd's three differ by the no-CD patch (D2600).

**D12600 (vsdk/unnamed_vif_0x5329f0.cpp; m-1020b worker 2). The segment search keeps the found index in `seg`.**
vif_emitter_spawn_point (0x532cb0) and vif_curve__evaluate (0x533ca0) send the search loop's normal exit straight
to the "last key" block and test only `i < 0` after a break (D10311 found no form for it). Build 8966 gives that
from `int seg = -1; for (i = 0; i < n - 1; i++) if (...) { seg = i; break; } if (seg < 0) ...last key...`: on
the normal exit seg is the constant -1, so the test is threaded away; after the break seg is i. Same behaviour
(the old test `i >= n - 1 || i < 0` holds exactly when no segment was found). Further shapes: the byte count is
tested in place (`path->count == 1`, `path->count > 2`: `cmp bl, 1` / `cmp bl, 2; jbe`) and `n = path->count`
is read after the single-point test; spawn_point converts the index through `float fi = (float)i` (fild / faddp,
not fiadd) and writes the line case as an `else` block with `vector3 dir = points[i + 1] - points[i]` (the
temporaries' slots as the binary); curve__evaluate indexes the last key with `c->count - 1` again. Both are byte
matches now. The same `seg = -1` shape may fit other "search, then test the index" loops.

**D12601 (vsdk/gr_opengl.cpp; m-1020b worker 2). Packed-pixel converters: source copy first, destination in the
guard.** gl_convert_8888_to_rgba4444 (0x54e050) and gl_convert_888_to_rgba5551 (0x54df90) load src before the
`tex_h > 0` test and dst after it: `ubyte *s = src; int y = 0; if (tex_h > 0) { ushort *d = (ushort *)dst; do {
...; y++; } while (y < tex_h); }` (D11200's guarded do-while plus the walking copy of src). Both byte matches.
gl_convert_a8_to_rgba8888 (0x54d5c0): `*d++ = ...` in both arms gives the binary's registers; only the operand
order of `mov bl, [esi + ecx]` (ours `[ecx + esi]`) is left (src[x], *(src + x), x[src], casts, a row copy: no
change). The 4444 / 1555 scaled converters: the binary forms the texel address with `lea eax, [ecx + 2*eax]` and
loads through it; five source forms (element pointer, cast, index local, byte offset, ushort local) give the
folded `[edx + eax*2]` (not reproduced).

**D12602 (vsdk/unnamed_vif_0x5329f0.cpp; m-1020b worker 2). Closer, still differing.** vif_draw_particles
(0x533620): the binary passes operator+'s returned pointer (EAX) to gr_get_point_fog_value with no named world
vector; `&static_cast<const vector3 &>(a + b)` (legal C++, mingw accepts it) gives that (92 -> 9 differing lines);
left: the register rotation of the gr_bitmap_3d arguments (mode ECX, s EDX, pt EAX; ternary, local, if / else,
inverted test tried). vif_instantiate_level (0x5337a0): the static branch as `int used = vif_particle_used; if
(desc->count + used > MAX) { compact; used = vif_particle_used; } int total = desc->count + used; ...
vif_particle_used = total;` gives the binary's prologue and desc in EDI (103 -> 98 lines); left: used / count in
EDX / ECX swapped, the zero register, plist_remove's block order (the macro and an inverted test give the same
code). vif_record_get_bounds (0x533aa0): the length through a float local (`float l = (...).len(); farthest =
float_max(farthest, l);`), 30 -> 2 lines; left: the int-to-float temporary of `1.0f / count` uses the dead `rec`
argument slot where the binary uses `radius`'s (six forms tried). Not reproduced (tried): itemtrade_count_party_rows
(volatile store and load forms, pointer, early return, local), boss_threat_add (not retried), level_trigger_point_
left_of_line (the binary's frame has 8 unused bytes at the top; unused locals, blocks, aggregate init, arrays give
none; a copied temporary gives 0x28 but other code), mp_handle_swi_path (the walk flag stored to the dead data
slot and reloaded as a dword; locals, casts, ternary, an inline wrapper give a register), party_set_level (the
`[eax + ecx + 0x93]` operand order: info local, x[p] forms), chat_input_handle_key (an Enter fall-through into
Esc: same code).

**D12700 (game/unnamed_party_inventory_0x46b460.cpp). character_use_inventory_item's break roll in its own
block.** The fractional-disposable case is `if (ce->disposable > 0.0f) { float r = frand(); if (r < ...) goto
remove; } entry->item->charges--; goto done;`: the binary calls frand before reading entry->item again (the
`||` form kept entry->item in EDI across the call). Same behaviour, NaN included (the binary's `test ah, 0x41`
sends an unordered disposable to the charge decrement, as `> 0.0f` does). Now a byte match.

**D12701 (game/unnamed_evil_urath_0x4c4170.cpp). evil_urath_ai_leap_rise / leap_travel are if / else, not
early returns.** The binary's frames: in leap_rise the rising branch has top at the bottom slot and its own
distance above it, the falling branch its distance at the bottom slot and the vector2 above (two distance
locals, one per branch; top at function scope); leap_travel's 24-byte frame comes with the second half in an
else block. Same behaviour. Both byte matches.

**D12702 (vsdk/unnamed_ix_0x53ac20.cpp). ix_linesegment_boundingbox: two coordinate locals.** The binary keeps
each face's two computed coordinates in the same two slots (the out and min argument slots), so the source has
two locals u, v reused by the three faces instead of x, y, z. This was the remaining difference noted in D6305
(not a build difference). Byte match.

**D12703 (vsdk/unnamed_ix_0x53ac20.cpp). ix_linesegment_sphere: one shared false exit.** The range test is
`if (-r > proj || len + r < proj) return false;` and the hit test an `if (d < r) { ... }` around the rest with
`return false` after it (the binary jumps to one epilogue); the zero-length case measures into t, which shares
the b argument slot with proj and t as in the binary. Same behaviour. Byte match.

**D12704 (vsdk/unnamed_vfx_channel_0x55b440.cpp). The force record reads the frame count in place.** The
binary has no call to vfx_node::num_frames in fx_rec4c::get_transform and fx_rec4c::update (this unit is /Ob0,
so the inline member would be called): the source writes `f->header.frame_word >> 8`. get_transform is now a
byte match. update also gets the binary's `if (!((float)n > t))` layout and the quaternion through a named
local (`quaternion rq(...); rot = rq;`, the binary re-takes its address); it still differs in the frame slots
of its temporaries (the binary gives every named local and temporary its own slot). Same behaviour.

**D12705 (vsdk/bmpman.cpp). bm_release_from_mark: the list head in a local.** The binary reads
bm_used_list.next once and again after each bm_release, and starts the second loop from that register; a local
`first` updated after each release gives it. Same behaviour (only bm_release changes the list). Byte match.

**D12706 (vsdk/bmpman.cpp). bm_free_data clears an animation's frames through a walking pointer** (`f = e;
... f++; f->data = NULL; f->palette = NULL;`): the binary derives the loop pointer from &e->palette kept in EDI.
Same behaviour. Byte match.

**D12707 (notes only; worker 3 of the 2026-10-20b round). Tried without a match:** state_title_enter (the AND of
game_flags is scheduled before the compare in the binary; 10 forms), mp_levelselect_drag_scrollbar (the term
order is right with `thumb - pos + (mouse - start)` but the list index sits in EBX in the binary; same as
mp_charselect_drag_scrollbar), gr_get_frustum_planes (EAX / ECX swap of count and index), bm_prefer_vbm and
bm_lock_tga (the binary saves all registers before the first test), bm_set_reduced (`and eax, 0xff` of the
level count, D5905), bm_load_internal (`reduced` in BL in the binary, on the stack here; same frame otherwise),
bm_lock_pcx (the binary scales the 9 * index by 8 in each address), gr_line (the integer copy of the clip value
is loaded earlier), ix_box_oriented_box_oriented (the product order of the bounds is normalised: 18 orders of
test 1 give the same code), seg2d_intersect, gr_project_vertex (the flags in a local fixes the z <= 0 exit; the
flags & 3 exit is placed at the end by 8168 whatever the form), gr_line_3d (local copies v0 / v1 and `ret` as a
real variable give the binary's 8-byte frame and memory `ret`, not yet its register order),
gr_cull_bounding_box_transformed (the binary constructs no plane for the transformed one).

**D12800 (game/unnamed_netsession_0x440b40.cpp, w4 m-1020b). multi_process_client_status 0x441150: the status is
read into a local before the packet header.** `int status = p->type;` ahead of MP_BUILD_HEADER gives the binary's
load of +0x14 before +0x1c and its store of the status (data+7) before the id (data+3). Same behaviour: nothing
between the read and the store writes the player record. Byte match (strict).

**D12801 (game/new_spell.cpp, w4 m-1020b). spells_set_page 0x42a030: `spells_page = page;` is the first
statement**, before the six button-flag clears (8966 then loads the argument first and schedules its store among the
flag stores as the binary does). Independent globals, same behaviour. Byte match (strict).

**D12802 (vsdk/unnamed_vfx_0x536950.cpp, w4 m-1020b). vfx_instance::get_node_world_transform 0x537a10: the child
loop decodes each tag into its own block-scope `int kind, list, sub;`.** The binary's frame is 12 bytes larger
(0x15c) and its loop addresses kind / list / sub at other slots than the function-scope ones; declaring them in the
loop block (before `matrix corient`, after or before `vector3 cpos`) gives the frame and every offset. The outer
values are not read after the loop, so behaviour is unchanged. Byte match (strict).

**D12803 (notes only, w4 m-1020b). Tried and left (details in w4-NOTES.md).** mp_player_alloc (the &p->prev
reload of D7006; local unlink, reference and inline-function forms give the same code), mp_reset_net_stats (the
second zero register; chained stores, timer placement), character_use_skill_on (the shrink-wrapped push ebx of D8701;
goto, nested-switch, if / else and default-fallthrough single-exit forms all push at entry), level_vis_reset (the
-4 biased pointer with jb; index, unsigned index, pointer and do-while loops), snow_start (the row advance of the
record pointer before the inner counter; a separate next-row pointer gives everything but the order of two
instructions), levelrender_add_items / add_containers / add_lobj (the binary keeps d.y and d.z on the FPU stack and
squares copies, the pattern of len()'s sqrt argument without the fsqrt; no float / double / len_sq / dot form gives
it), levelrender_add_effect_entry (statement order: type and fx before the depth gives the integer stores, the
depth's x, z, y order with fmulp into dx not found in 48 term orders and dot forms), ui_inputbox::set_valid_chars /
set_invalid_chars (one function-scope `lo` gives the binary's 0x8c frame but 8966 then cross-jumps the three range
blocks, which the binary keeps), levelrender_effect_apply_vis (the tag offset in the first argument's dead slot,
D7006), tiger_path_to_target (out-of-line vector3 constructors and copies; inline_depth(0..2) does not give the
binary's mix), levelrender_draw_speech_text (the binary jumps to a shared return where 8966 duplicates the epilogue;
four branch shapes tried). levelrender_add_effects is closer (a name local for the _stricmp argument and
`vector3 c = fx->pos + vi->pos.unrotated(fx->orient);`), still differs in the node loop's layout.

**D12900 (include/vsdk/math/vector.h; m-1020b worker 5). HEADER CHANGE: vector3::len rounds to float, then
promotes, before its float return.** Every unoptimised living_entity caller of the inline len stores the result
twice (Sum.exe get_speed 0x457060: `call __CIsqrt; fst [ebp-0x10]; fst [ebp-8]`; stopping_distance 0x45a840:
`fst [ebp-0x3c]; fst [ebp-8]`; move_frame 0x45d48d the same), and the frames have the extra 4-byte slot. Every
float-valued spelling tried (plain, implicit conversion, `float()`, `(float)(float)`, a named local, `r = ...`,
an inline fl_sqrt helper with a float or double parameter, nested inline wrappers) gives one store; a double
return gives a QWORD temporary. `return (double)(float)sqrt(...)` reproduces both stores: a device, not the
recovered original spelling; the value is the same float length. Optimised units drop the cast (full strict
sweep: 0 of 5,255 lost). The COMDAT 0x4bc290 sits in a comment block and is not compared. Wins with it:
anim_speed_at_blend 0x456ed0, get_speed 0x457060, and (with renames below) stopping_distance 0x45a840,
distance_to_reach_blend 0x45a930, distance_to_reach_speed 0x45aa20. mingw: all 259 .cpp, only the 9 known
errors of unnamed_decals.

**D12901 (game/living_entity.cpp; m-1020b worker 5). /Od frame and operand fixes.** stopping_distance: the
length local renamed `l1` (D6101 bucket 7, declared after vec, so it lies above it at -8). distance_to_reach_blend /
_speed: the result goes through a `dst` local (bucket 8, the binary's -0x20 slot) before the return.
ai_travel_offset 0x455e00: `pos + offset` (pos is operator+'s object); same sum. All four byte matches.

**D12902 (notes only; m-1020b worker 5). Tried, still differs.** combat_approach 0x4bda50: `? true : false`
returns give the binary's test / setne (LCS mnemonic difference 30 -> 14, kept); the boss branch's info reload
and the goal_do_cast return (neg / sbb / inc then test / setne; five spellings) remain. set_cipher_key
(indexed slots get the binary's per-branch address forms, but the key test keeps a zero register: 12 forms),
get_async_status (the unfolded `add eax, file_slots`: 5 forms), get_speed-style forms done; scale_stats_to_level
(fidiv: 8 divisor spellings), Character::say (loop tail scheduling: 6 forms), luminar_boss_load (FPU register
choice: 6 forms), gr_env_map_uv (the `lea` of the 2 * normal temporary: 5 forms), move_frame (the third row of
matrix::operator= is called out of line in the binary: inline budget). Not retried: the anim_remap_in copies
(D7702).

**D13000 (game/new_saveload.cpp; m-1020b worker 6). A clamped row count compares the global, not the local.**
saveload_draw_list 0x426a20 keeps `rows` only in its stack slot: `mov edx, [saveload_num_saves]; mov [esp+4], edx;
cmp edx, [page]; jl; mov [esp+4], ecx` before the register pushes, and reloads it for the loop test. `int rows =
saveload_num_saves; if (saveload_num_saves >= saveload_list_page[ri]) rows = saveload_list_page[ri];` gives that
(as do an if / else of the two stores and the ternary); testing the local (`rows >= ...`) makes 8966 keep it in ESI
as well. Now exact. Same behaviour. The same `rows` clamp in saveload_handle_input (0x425de0) does not change that
function (it differs earlier: the `saveload_selected_row < 0` return 0 is merged with the KEY_ESC tail at 0x426256).

**D13001 (game/new_saveload.cpp; m-1020b worker 6). saveload_draw_info 0x426d10: the am / pm copy is an if / else of
two strcpy calls.** The binary computes `lea ebx, ampm` before the hour test and selects the source string in EDI
(one inline strcpy); the ternary argument scheduled the lea after the selection. With the if / else every instruction
and register is the binary's; still differs only in two stack slots (w and ampm swapped, and 4 bytes between buf and
line: the binary's frame is 0x25c, ours 0x258). Declaration order (5 orders) does not move them. Same behaviour.

**D13002 (notes only; m-1020b worker 6). Tried, still differing.** snd_set_volume / snd_set_volume_in_group /
snd_set_pan / snd_set_group_volume (0x521680-0x5218e0): the binary's clamp lays out `jne store0; ...; je store1; jmp
done; store0: ...; jmp done; store1:`; if / else-if, nested ifs with an empty then, ternaries (self-assignment
dropped), gotos and a switch all give the plain layout. pf_read_edges 0x49c970: only `lea ebp, [eax]` against
`mov ebp, eax` (a trip-count copy); 15 loop and count forms either keep the copy or add an entry test.
seg2d_closest_point 0x49db20: the binary's frame has 8 more bytes (0x18); vector2 constructor / operator forms give
the frame but copy through a temporary, dx / dy locals do not. pf_find_cell_crossing 0x49ecb0: frame 0x28 against
0x20 (an 8-byte slot below the EH record that nothing touches) and found / hit / seg slots; a vector2 difference,
unused locals and declaration orders do not change the frame. options_init 0x41f0f0: `test al, cl` operand order and
the first options_edit read forwarded from Summoner_options (memcpy, statement order; a volatile read of the
argument gets the load but as fld / fstp). hud_drag_icon_begin 0x410570: the binary ORs active into the loaded word
and masks it (`or al, 1 ... and eax, 1`); all 24 store orders, `|=` forms, signed fields and an int OR fold to the
constant 13. ca_character::report_memory 0x541020: the skeleton sum (pos * 64 first); 15 groupings, split
statements and temporaries compile identically. cmdline_take_argument 0x53e250: is_done stored inside the inline
strcpy; stores before / after and volatile forms move registers instead. ls_ikaemostopint_init 0x4f9ec0: a bool
local for the constant 1 still gives immediates. credits_draw_lines 0x408ba0: `volatile float alpha` with the clamp
as `a = alpha; if (!(alpha < 1.0f)) a = 1.0f;` reproduces the memory-resident alpha and the clamp except two things
(the 1.0 store is scheduled before the resolution_index load, the 0 case is inline where the binary puts it after
the ret); not kept (volatile, still differs). pick_attack_slot 0x472780: as D10506 (get_center x operands).

**D13100 (m-1020b worker 7; game/new_charselect.cpp). mp_charselect_drag_scrollbar (0x407e10) byte match.** The list
index is read from mp_charselect_drag_scrollbar_index at every use (no local: with one, 8966 holds it in ESI where
the binary has EBX), the row count is loaded again after the clamp to 0 (`int count = row_counts[..]` placed
there; the binary loads it into EDI then), and the drag offset is `thumb + (mouse_y - track_y - start_y)` (as
assess_drag_scrollbar; the original order `mouse_y - track - start + thumb` loads the operands in another order).
Same behaviour. mp_charselect_page_down (0x407410) still differs (rows / page register order; six forms).

**D13101 (m-1020b worker 7; game/unnamed_interface_0x411270.cpp). ui_message_box_draw (0x414270) byte match.** The
title position is computed into x and y before gr_set_color_ptr(&ui_color_text) (the binary loads the box position
and adds the title offsets before that call; globals cannot be moved across a call). ui_choice_popup_draw
(0x413c70): `x = cond ? .. : ..; x += ui_choice_popup_x;` in the choice loop brings it from 38 to 6 differing
lines (kept); the rest is scheduling around `add esp` after calls (the title's x / title_gap and the loop's
popup_x load), not reproduced in 17 forms; ui_choice_popup_show has the same post-call load pattern (6 forms).

**D13102 (m-1020b worker 7; game/characterinfo.cpp). characterinfo_load_party (0x4450c0) byte match.** (1) The move
free-list and species loops are index loops (`for (i = 0; i < N; i++)`): VC6 strength-reduces them to a pointer
compared signed (`cmp eax, end; jl`, as in the binary); pointer loops compare unsigned (`jb`). (2) In the cast
loop cast_spells is stored before num_cast_spells (the other order makes 8966 keep a zero register in EBX for the
whole function), and the species is read as `char_infos[i].species` into a local after the two stores through
`info` (read through `info` the load is scheduled above the stores and the induction pointer is biased to
another field). Same behaviour. General: a binary pointer loop ending with a signed jl is an index loop.

**D13103 (m-1020b worker 7; game/combat.cpp). living_combat::roll_defense (0x464100) byte match.** The two clamps
at zero are `if (0.0f > x) x = 0.0f;` (the binary loads the constant first: fld 0 / fcomp x / test ah, 0x41),
and the slow / disabled reduction is applied to `dodge` in place (`dodge -= 0.2f; clamp; dodge *= 0.5f; if
(loco_blend > 0.5f) dodge = 0.0f; d = dodge;`): the if / else into d gave the else block first. Same behaviour.

**D13104 (m-1020b worker 7; game/combat.cpp). living_combat::apply_melee_hit (0x465df0) byte match.** spawn_blood(dmg)
is called at the end of each branch of the MP_CLIENT test instead of once after it: the binary's client branch
pushes dmg from its frame slot and jumps into the server branch's call (cross-jumping), where the server branch
pushes EDI. Same behaviour. Note: tools/vc6-side.mjs lost the first instruction (push ecx at 0x465df0) of the
binary here, so its side-by-side was shifted by one line; check the alignment before reading the stars.

**D13105 (m-1020b worker 7; notes only). Tried without a match:** camera::set_mode (0x479930: the binary compares
`cmp [esi], edx` and reloads mode at the join, ours keeps it in ECX; volatile reads and operand orders),
living_combat::ai_choose_special (0x463d00) switch forms (6 case orders; the push test block after the epilogue stays),
characterinfo_load_party's zero-register forms before D13102 (8 forms), roll_defense's `||` block forms
(8 forms before the in-place form). Not attempted in depth: melee_damage (frame 0x294 against 0x214: the
COMBAT_SELF_CAST temporaries get separate slots), resolve_exchange, trigger_special / reflex_ability (D5700,
D5506), mp_charselect_handle_click (the binary shifts button_def flags unsigned: `shr`, ours `sar`; the field
type would change other units), gr_poly_get_section (sw kept in bm's argument slot), ui_message_box_show (shared
`mov al, 1`).


## m-1020b worker 8 (D13200-D13204): gr_poly replay / clip, dialogue box, ai_move cast, mp objects, jade temple

**D13200 (vsdk/unnamed_gr_poly_0x5450c0.cpp). gr_poly_buffer_flush: inline expansion off, one a2 / z, an endless
replay loop.** The binary builds the 25-vertex array with `vector constructor iterator` (0x404880) and gr_vertex's
implicit constructor, as gr.cpp's /Ob0 functions do (D12103), so the function sits between `#pragma
inline_depth(0)` and `#pragma inline_depth()` (as gr_poly_set_texture / gr_poly_rgb, D11600). `nv = 0` comes before
the array (the binary clears EDI before the iterator call). The two begin opcodes read into one function-scope a2
and z: the binary cross-jumps their gr_poly_begin calls (BEGIN_DEPTH pushes depth and 1, then jumps into BEGIN's
pushes), which needs the same slots. The replay loop is `for (;;) { switch (gr_poly_buffer_get_byte()) { ... case
END: ...; goto next; } }`: every other opcode and an unknown one jump straight back to the read (the old `do { op =
...; switch (op) } while (op != END)` kept op in a register and tested it). Same behaviour; now exact. The same
pragma also puts gr_poly_alloc_clip_vertex / gr_poly_free_clip_verts / gr_poly_reset_clip_verts out of line in
gr_poly_clip_edge (0x545ac0) and gr_poly_clip_and_draw (0x545870), as the binary calls them: clip_edge is down to the
base / index order of three uv addresses (`[esi + eax]` against the binary's `[eax + esi]`; operand orders, element
pointers and loop forms tried); clip_and_draw now keeps the list in EBX and nv in the list argument's slot (the list
copied into a local at entry, an element pointer per vertex) and differs only in the shrink-wrapped prologue (the
binary pushes only EBX before the texture-count test; early return, wrapped body, local count tried).

**D13201 (game/unnamed_dialogue_box_0x4d6660.cpp). dialogue_box::add_text: one shared `return NULL`.** The binary
returns NULL from one block (`xor eax, eax` and the epilogue) placed after the label branch of the ring-not-full path,
and the ring-full path's failed alloc_label jumps there. Written as `if (lb) { ... } else { fail: return NULL; }`
with `goto fail` in the ring-full path. Same behaviour; now exact.

**D13202 (game/unnamed_ai_move_0x4b1410.cpp). ai_state::cast_spell_update: summon test first, the velocity through
operator=, both failure tails written out.** `int summon = ...` before the profile_label (the binary stores the EH
state after the summon_find_charinfo call); `vector3 vel; vel = owner->instance->sum_root_velocity();` gives the
binary's partly copied temporary (x / y read from the returned pointer, z copied to the temporary with integer
moves); the `target = -1; set_mode(prev_mode, prev_submode); return;` tail written in both failure places (8966 then
cross-jumps the second into the first, and clears flag bit 2 with `and dword ptr`). Same behaviour; now exact.

**D13203 (game/level_script_jadetemple.cpp). ls_jadetemple_action: the "lost item" block is a static inline helper
with its own name buffer.** With both 256-byte buffers in the action, /O2 places them the other way round whatever the
declaration order or block scope. A `static inline void jt_lose_item(const char *name, char *line)` with a local
item_name, called four times with the action's line, gives the binary's frame (line at esp+8). Same behaviour; now
exact. (A helper with both buffers local fixes the last three blocks only.)

**D13204 (notes only). Closer or tried, m-1020b worker 8.** Closer, kept (same behaviour): mp_handle_status_add
0x480d70 (reads in the order value, status, ms, spell, source: integer registers now right, 20 -> 12 differing
instructions; the value still goes through ECX / EDX instead of EAX / ECX and `this` is reloaded earlier).
mp_send_obj_update 0x480530 is exact through MP_ADD_FLOAT for the two speeds (the block-scoped float temporary gives
the binary's slots); the same macro does not change mp_send_new_lep 0x47fdf0 (the packet counter in ESI and the
float stores' order). Tried, no form found: mp_handle_ai_disable (from in ECX, not EAX; case order, if form, types),
last_visible_line and both draw functions of the dialogue unit (the ring index register; local first, inline
helper, volatile read), steer_jitter (two vector temporaries' slots; operator= and named temporaries make it worse),
mem_report_line (8966 orders `65 - strlen(text) - len` the same in every written order and type), cutscene_script_load
(the EH epilogue placement; goto-fail and if / else forms leave the code unchanged), tentacle_beast_boss_load (one
`mov ecx` scheduled before `push 0x87`), tentacle_beast_ai_frame (exit block placement; goto labels and an else-wrapped
body leave it unchanged), script_action_combat_mode (skip / teleports_only register choice; declaration orders,
types, loop forms), mp_send_new_lep, path_closest_point (frame 0x58 against 0x60). Not attempted in depth: the
skeleton, vfx_mesh and the large script_internal / ai_move functions, multispell (register choice, logged by
m-1019 w6).

**D13300 (include/engine/ai/unnamed_avoidance_0x4a6710.h, game/level_script_sewers.cpp,
game/level_script_khosani_lab.cpp, game/unnamed_avoidance_0x4a6710.cpp; m-1020b worker 9). HEADER CHANGE:
avoid_spline_def's end points are a plain x / y pair without constructors (`avoid_spline_end`), not vector2.**
Neither ls_sewer_create_avoid_spline (0x4ee3c0) nor script_build_avoid_spline (0x4ddc60) calls `eh vector
constructor iterator` for the def's arrays, and ls_khosanilab_path (0x257ebc0) has no static initialiser; with
vector2 (user default constructor, D3706) VC6 calls the iterator for both arrays. The sewer corners are
avoid_spline_end locals set in their use order (c, d, a, b): with a POD the binary's stores into the corner slots
and the dword copies through registers follow (24 corner orders x 2 component orders searched, 3 match; the use
order is one). Same layout (8 bytes, x then y), same behaviour. avoid_spline_create passes `(const vector2 *)&p0[i]`
(avoid_spline_point_create still reads vector2s), ls_khosanilab_puzzle_add_point copies `*(const avoid_spline_end
*)a` (still exact). ls_sewer_create_avoid_spline now exact; 0x4ddc60 still differs (other reasons). mingw: all .cpp
as before (only unnamed_decals); full strict sweep 0 lost.

**D13301 (game/unnamed_ai_think_0x4b5140.cpp). ai_score_target reads q->self and q->candidate through the query
every time.** The binary reloads `[esi]` / `[esi + 4]` before each use (no register copies of the two characters,
frame 0x1c); with `c` / `self` locals 8966 keeps them in ESI / EDI. The facing value is `d.dot(q->self->orient.fvec)`
(the binary's z, y, x order; every hand-written order gives x first). Now exact.

**D13302 (game/unnamed_avoidance_0x4a6710.cpp). avoid_prim_from_segment: axis_b through an inline setter.** The
binary loads axis_a.x, then axis_a.y, negates and stores both straight into axis_b: the evaluation order of the
arguments of an inline `set(x, y)` (right to left) stored in place. Assignment from `vector2(-y, x)` builds a
temporary, and component assignments in either order load y first. `vec2_set(vector2 &, float, float)` is a file
helper (name ours; it may be the original's vector2 member, by analogy with vector3::set; not added to vector.h, no
other evidence). Now exact.

**D13303 (game/unnamed_ai_think_0x4b5140.cpp). think_party_update's step-aside block.** `vector3 d; d = owner->pos
- c->pos;` (temporary plus copy, D10302), the two facing tests as `d.dot(c->orient.fvec)` / `d.dot(c->orient.rvec)`,
and `d` itself reused for the step-aside point (the binary's frame 0x48: the destination shares d's slot; a
separate `dest` local adds 12 bytes). Now exact.

**D13304 (notes only; m-1020b worker 9).** Kept, closer but still differing (behaviour identical):
avoid_prim_overlap and avoid_box_box_overlap test `fabs(dot) > sum` (the binary evaluates the projection first;
mnemonic LCS distance 42 -> 3 and 24 -> 0; box_box still has a and b in swapped registers, prim_overlap the mixed
operand order of one dot), ku through `b->axis_a.dot(*u)`; pick_attack_target's distance in a float local (the
binary's fxch; the -1 constant in EBP remains); lobj_ray_test_faces / lobj_sphere_test_faces with
`ray->local_dir.dot(face->normal)` (first difference moves to the third term). Tried without a match: all 8 term /
operand orders of every two-term dot in prim_overlap and box_box_overlap (8966 emits one order whatever the source),
parenthesised terms; worldmap stage9 (outer / inner inversions, a name pointer, duplicated tails, a three-way form:
the desert-night block stays next to the desert-day block), stage15 (24 declaration orders, 10 renames, roll
variable reuse: the frame slots of dist and r2 stay swapped), avoid_spline_point_get (pointer and early-return
forms), pick_attack_target's final test as one if / else-if.

## m-1020b worker 10 (D13400-D13404)

**D13400 (vsdk/parse.cpp). file_write_text_block (0x517180) keeps one index for the search and the advance.**
`int i = count; if (count > 70) { i = 70; while (i--) { if (CR or space) { count = i++; break; } } if (i < 0)
{ for (i = 70; i < count; i++) ... count = i++; break; } } write_line(text, count); text += i;`. The binary's
`mov eax, esi; dec esi; test eax, eax` is the post-decrement test, and the found case tests the incremented index
with `inc esi; jns` (0x5171bf). Same behaviour as the old two-variable form. Exact.

**D13401 (vsdk/parse.cpp). file_blast_into_buffer (0x517220): both stores written above the strcpy.**
`file_blast_buffer = buf; file_blast_size = size; strcpy(file_blast_filename, name);` - VC6 schedules the two
stores after the inline strcpy and between the call's pushes (0x51728d / 0x517293), as the binary. Exact.

**D13402 (game/level_script_khosani_lab.cpp). choose_turn (0x4f8190): offset cells are block-local copies.**
Each grid index `[cell +/- d]` is `ls_khosanilab_cell n = *cell; n.x += d.x; n.y += d.y;` in its own block, then
`grid[n.x][n.y]`. A copied struct keeps x and y as separate sums before `5 x + y` (`lea ecx, [ecx + 4*eax]; add
eax, ecx`); with the plain expression 8966 reassociates the 3-term sum and with int locals it still folds y into
the outer sum. The same form also lets the state-0 kind-0 and the else kind-2/3 tails cross-jump (0x4f822b ->
0x4f8385) as in the binary. Exact.

**D13403 (game/level_script_khosani_lab.cpp). emit_segment (0x4f8400): the bend points through v2_set.**
`v2_set(&g, 2.2f, p0->y)` (D12100's two-argument setter) instead of two member stores: p0->y is copied with
`fld [esi+4] ... fstp` around the pushes, the binary's code, and the two border cases then cross-jump into the
y = 0.2 case's tail (0x4f850b -> 0x4f87a4, 0x4f865e -> 0x4f8795). Exact.

**D13404 (game/multi_door.cpp). door_pick_by_mesh (0x4aaa10).** `bool open, locked` belong to the success block
(the frame is 0x108: `open` reuses the door pointer's slot, 0x4aaad9), and the test is
`ch->get_skill(SKILL_PICK_LOCK) + 0.05f >= (double)d->lock`: the double operand loads the lock onto the FPU stack
(`fld [edx + 0x58]; fcompp`, 0x4aaacb) where a float compare gives `fcomp [edx + 0x58]`. On the x87 the values
are compared at full precision either way (same result). Exact.

**D13500 (game/unnamed_shadows_0x4a37d0.cpp; m-1020b worker 11). shadows_render reads both counts once.** The
binary counts the records down from num_blob_shadows read once and tests the 3-bit lobj_count unsigned (`jbe`) after
reading it once per record: `count = num_blob_shadows; for (i = 0; i < count; i++) { s = &blob_shadows[i]; n =
s->proj.lobj_count; for (k = 0; k < n; k++)` with k and n unsigned; the face-bit loop shifts in its increment (`bit++,
m >>= 1`: `inc edi` before `shr edx`). Byte match. Same behaviour (nothing in the loop changes either count).
lobj_raycast_draw_lists (same unit, still differs): the stage byte goes through the `blocked` parameter itself
(`*(char *)blocked`, the binary stores &local_stage into the argument slot) and `n` is block-scoped; now only the
frame (first in hit's argument slot in the binary: 12 names and a hit alias tried) and an ESI / EDI swap differ.

**D13501 (game/spells.cpp). spell_fire_missile returns early without a missile slot.** `if (missile == -1) return
h;` before spell_missile_remove gives the binary's single h register (EBX) and its one epilogue; the nested if kept
two copies of h. Byte match, same behaviour.

**D13502 (game/spells.cpp). spell_probe_ground_point adds the effect position in place.** `v = pos.unrotated(..);
v += fx->pos;` (no separate result vector: frame 0xc0) and `lobj_ray ray;` declared where it is set up (its
constructor's min_radius store comes after the unrotate call in the binary). Byte match, same behaviour.

**D13503 (game/multisave.cpp). mp_player_file_create's zero register.** `int err = file_make_path_dirs(..); if
(err)` gives `cmp eax, ebx` (EBX the zero register), and the species switch's default clears the pointer before the
count (`prog = NULL; num_prog = 0;`: the binary stores EBX into prog and leaves the count register). Both needed;
byte match, same behaviour. mp_player_file_load (still differs): the equipment loop's unequip result shares old1
with character_equip_item's first output (the binary's slot at [esp+0x10]); now only the two savegame_state calls
at the end differ (the binary loads ECX = &savegame_state before the species argument; species / info / pointer
locals give the same).

**D13504 (notes only; m-1020b worker 11). Tried without a match.** spell_roll_amount (the binary tests count, then
`dec eax / lea esi, [eax+1]`; 20 loop shapes, `for (i = count - 1; i >= 0; i--)` closest with the dec before the
test), mp_player_file_read_summary (EDX against ECX in the version-8 flag fix-up: 7 forms), mp_player_file_write
(register rotation in the inventory loop: 3 forms), localize_load_names (case 1 placed after the epilogue: switch
orders, if / else, a name pointer), localize_remap_charset (the loop test on *text before p = text: 2 forms),
shadow_project_face (`world = ..unrotated(..); world += l->pos` takes the frame to 0x1c0 of 0x1b8; the binary
passes `&(w + l->pos)` as a temporary to gr_rotate_vertex; reverted), polygon_clip_2d (n kept in EBX, flip in n's
argument slot: 3 forms), mp_handle_item_drop (the matrix built from three argument temporaries gives the binary's
fvec-first stores, but the dequantized position copy and the data reads still differ; not applied),
vector3::random_* (D-noted: fmulp; 5 more forms). conversation::clear_speaker still needs the speaker member
declaration of D10904 (1).

**D13600 (vsdk/gr_direct3d.cpp; m-1020b worker 12). d3d_enum_texture_format_callback (0x53a1d0) exact.** The bit
counts clear the count after the first loop and, for a zero mask, in an else (D3D_MASK_BITS); the bump case starts
du and dv at 0 and has no else (D3D_MASK_COUNT; the two xors before the first test). The colour total is written
`r + g + b` twice (in the test and in the store) instead of through a local: with a local, build 8966's
normalisation emits lea / add, the repeated expression gives the binary's add / lea. Same behaviour.

**D13601 (vsdk/unnamed_vfx_emitter_0x55ff00.cpp). fx_recb0::get_billboard_axes (0x562840) exact: the first tip uses
fade.** The overwritten first c tip is scaled by the last argument fade (0x562947 loads [esp+0xc0]), not by size as
the source had. The store is dead (c is assigned again at once), so behaviour is unchanged; fade still has no effect
on the triangle.

**D13602 (same file; notes). fx_recb0::apply_forces (0x5605f0), closer.** The plane force passes the address of the
sum `pos + v64` (`&static_cast<const vector3 &>(pos + v64)`, standard C++: the temporary lives to the end of the
call) instead of a named local; the code now matches up to the 0.0625 factor, which the binary loads from .rdata
(0x58232c) into ECX and pushes where ours pushes an immediate (tried `(float)0.0625`, `1.0f / 16.0f`,
`fixed4_to_float(1)`, `/ 16.0f`).

**D13603 (game/new_automap.cpp). automap_load_bitmaps (0x401ef0) exact.** Element pointers `b` / `d` in the button
loop as in journal / buysell; the two mask tile sizes through a float `m = max(scale_x, scale_y)` (the binary keeps
it on the FPU stack and pops it with fstp st(0)); the mask scale with `float sh = tile_h / mask_h` first: written as
one expression, 8966 orders the two quotients tile_w first whatever the source order (the strict check caught the
swapped globals). Same behaviour (float addition is commutative).

**D13604 (vsdk/unnamed_vfx_emitter_0x55ff00.cpp; notes). fx_recb0::spawn (0x55ff30), closer.** `prev = last_t;
first = (int)ceil(last_t);` (the binary copies last_t into prev with integer moves and converts last_t itself) and a
`rate` local for the three get_rate products (the binary multiplies with fmul st, st(1) and pops the rate after the
store). Now identical except that the binary stores prev after the fstp of the ceil argument and ours before (tried:
ceil(prev = last_t), (double) casts, a double local, prev declared in the block, a volatile store, declaration
order).

**D13605 (same file and vsdk/gr_direct3d.cpp; notes). Lock flag polarity.** fx_recb0::render_d3d (0x561900)
selects `vb_pos == 0 ? gr_d3d_lock_discard : gr_d3d_lock_nooverwrite` (the binary loads discard and skips the
nooverwrite load on zero, 0x56198e-0x5619a0); written that way the branch matches, same behaviour. Still differs
later (the end store after the loop test, fild / fdivp for the bitmap aspect, register choice). The reversed form
does not help gr_d3d_draw_primitive (0x538760): 8966 still preloads one flag where the binary has two branches.

**D13606 (notes; m-1020b worker 12). Tried without a match:** fx_recb0::init_particle (24 declaration orders; the
lifetime sum in 9 more forms: fiadd, or fadd st, st(1) leaving the value on the stack when the random value or
spread is a named float), fx_recb0::render (p / desc register swap: declaration order, loop forms), get_transform
(the binary evaluates each channel's frame term first and spills it; a local for it changes the frame), pcx_read_bitmap
(the binary pushes the zero arguments from EBP; `count` as the argument changes nothing, pos set after the buffer
read leaves only the xor esi placement), d3d_compute_pixel_format_shifts, gr_d3d_bitmap / gr_d3d_set_state
(constant-register and colour register choice), automap_draw_map (resolution_index re-read after the right edge).

**D13700 (vsdk/unnamed_listbox_0x52ca60.cpp). ui_listbox::get_vscroll_fraction (0x52e920): the bottom-aligned lo
through a float temporary, the font height in a local.** Sum.exe loads `last = entries.prev` (0x52e98d) before it
stores lo (0x52e995): `float t = ...; last = entries.prev; lo = t;` gives that order (assigning lo directly stores
it first). The line-stop branch multiplies `imul ecx, eax` with the call's result as the second operand: `int fh =
gr_get_font_height(last->font); hi = (float)((last->num_lines - 1) * fh + last->y);` (no operand order of the
inline call gives it). Exact (m-1020b w13); same behaviour.

**D13701 (vsdk/gr_glide.cpp, notes). Closer forms, still differing.** gr_glide_tcache_set (0x549560): no `slot`
local; `glide_tmus[tmu].is_bound(...)` / `.load_section(...)` in place give the argument loads before the slot
address and its spilled copy, and `switch (entry->sections[i].format)` (with `sec` for the case bodies) gives the
`lea eax, [ebx+ecx]` / `mov ecx, [ebx+ecx+0xc]` pair; every instruction now agrees except the stack slot of
`data`, which Sum.exe keeps in the dead `tmu` argument slot (0x5495f2 `lea eax, [esp+0x20]`; 8966's argument-slot
reuse, D6306). glide_record::alloc (0x549b50): `int bank = end >> 21; if (bank > (int)(addr >> 21)) addr = bank <<
21;` (the shifted value is shifted back, not masked), `used_tail = used_head` before the index store and `end = addr
+ size` reused as the loop limit leave one difference: Sum.exe adds the limit after the first `next >= 0` test
(0x549bf5), 8168 before it (no loop shape tried moves it: while / for / if + do, a new local). Same behaviour.

**D13702 (notes only, m-1020b w13). Tried without a match.** game_render_frame's `!(game_flags & 0x20)`: every byte
form (unsigned char casts, `~`, `== 0`, a local) loads a byte (`mov al, [game_flags]`), Sum.exe the dword then
`not al / shr al, 5` (D6703, the byte narrowing). character_use_skill's appraise test: Sum.exe calls get_skill
before the inline fsqrt and keeps the dead root store in the `target` argument slot; 8168 hoists the fsqrt above the
call (a skill local moves the call first but drops the dead store). editor_dialog_handle_event: a callback local with
a goto for the third button gives the tail byte for byte, but 8168 places button 0's block first and jumps forward,
Sum.exe falls into the shared call from button 0 and jumps back from button 1 (cross-jumping); not kept.
Crc32::update (pointer walk, operand orders, a value local), ui_listbox_entry::wrap_text (a copy of split),
ui_listbox::scroll_page_up (operand orders, locals), set_vscroll_fraction (num_lines local), ai_group_think_style2
(declaration orders, loop forms), state_game_enter (all case orders), conversation line_talker (case order, a local):
register choice or block placement only.

**D13800 (game/level_scripts.cpp). savegame::find_flag_level writes its search out.** 0x4d0240: the flag search
is an explicit inner loop that sets `index = i` only when found (the binary keeps the index in EBX, set to -1 once,
and the not-found path skips the store), and the found code (data_offset and the bit test) sits inside the level
loop with `return 0` after it. The savegame_find_hash inline gives an `or eax, -1` per level and a re-test after
the loop. Now a byte match. Same behaviour.

**D13801 (vsdk/unnamed_ui_static_0x527e20.cpp). ui_static::draw has the text code twice.** 0x5280e0: each branch
(hidden / normal) sets `c` itself (`c = colors + 3;` / `c = colors;`, the binary loads colors after is_hidden) and
ends with its own shadow and string calls. 8966 merges the two copies from the shadow call on, leaving the
binary's duplicated `mov ebp, c; test [ebp+0xb]; je; jmp` heads and keeping both justification blocks apart. With
one shared tail after the if / else, 8966 merged the vertical-justification cases across the branches (62 bytes
short). Now a byte match. Same behaviour.

**D13802 (game/level_scripts.cpp). savegame_location_allowed's switch assigns a result.** 0x4d4a50: every case
(and an explicit `default`) assigns `result` and breaks, with one `return result;` after the switch. 8966 then
duplicates the return into each case and does not merge the three identical `get_flag(..) ? 2 : 0` cases
(with returns in the cases it cross-jumps them, 36 bytes short). Initialising `result = 0` instead of the
default case keeps a zero in EAX before the switch. Now a byte match. Same behaviour.

**D13803 (game/level_scripts.cpp). Closer forms kept, still differing (behaviour unchanged).**
- level_item_add 0x4d0d00: the hashes read through `ubyte *data = level_item_hashes.data` with a byte offset
  `off` (4 upward, incremented before the compare) and the success branch first: same size and instructions;
  only the SIB base / index order of `[edx+ecx]` differs (ten address forms tried, as D12303).
- level_item_remove 0x4d0d60: a block-scoped `unsigned int *p` with `h = *p++` and `dst` for the memmove:
  remaining, 8966 puts p's init before the count test and in EDX (the binary: EDI, pushed after the test).
- read_level_items 0x4d33f0: the same pointer loop; only p / h register roles and p's init placement differ.
- get_speaker_bit / set_speaker_bit / get_topic_bit / set_topic_bit: the save info read through the
  CONVERSATION_SAVE_INFO macro each time instead of an `info` local (the binary indexes [esi + table] with the
  slot offset in ESI). get_speaker_bit then differs first at the search loop (8966 threads the not-found exit past
  the `i >= n` re-test; an inline search helper keeps the bit expression but not that), the others in the prologue.
- level_add_navpoint 0x4d5720: the navpoint as an index (`n = navpoint_count++; navpoints[n].`) and
  `ray.dir = vector3(0.0f, -1000.0f, 0.0f)`: frame and instructions identical; only the operand order of the
  z component's fld / fadd in `ray.start + ray.dir * hit.t` differs (`+=`, two steps, reversed sums the same).
- conversation_execute_action 0x4d5990: the cases in the binary's order (0, 6-12, 1-5, from jump table
  0x4d5dec). Remaining: the binary pushes EBX inside case 1 only; case-scoped locals do not change 8966's
  allocation (case 2 also takes EBX for p).

**D13804. Tried here, not reproduced.** eax_init (if / else, success-first nested, goto, result variable: 8966
always falls into the success block), effect_stream_start (volatile store, if-open form), gamecreate_handle_click
(case order, end switch), effect_stream_finish_entry (goto fail, success first), level_item_has /
has_conversation_data (temporary, paren, pointer compare: `cmp [eax], 0` stays), hot_button_get (k scope, volatile),
two_stage_get / write_party_state / sum_root_velocity (looked at only).

**D13900 (vsdk/matrix.cpp; m-1020b worker 15). A float sum of products keeps its term order when each product is
a local.** 8966 sorts the terms of a hand-written float sum and the factors of each product (the earlier
rounds' notes). matrix::determinant 0x5106a0 with one local per triple product (`float a = rvec.x * uvec.y *
fvec.z; ...; return a + b + c - d - e - f;`) gives the binary's term order whatever the factor order inside each
product; matrix43_interpolate_quat 0x511920 with the six products of the position lerp in locals (`float ax = (1.0f
- t) * pa.x, bx = t * pb.x; ...`) gives the binary's `faddp` before the return copy's `mov edi, eax`. Both exact.
Did not help (8966 still normalises): the three dot products of s3d_read_meshes' orientation check written out,
reversed or through vector3::dot.

**D13901 (game/new_inventory.cpp; m-1020b worker 15). inventory_load_bitmaps 0x414de0 is exact with
journal_load_bitmaps' shape:** `ui_button_state *b = &inventory_buttons[i]; const ui_button_def *d =
&inventory_button_defs[i];` at the top of the loop (the binary keeps &b->w / &b->h in the frame: 0x90, not 0x88).

**D13902 (vsdk/unnamed_ui_manager_0x52f570.cpp; m-1020b worker 15). ui_process 0x530330 is exact:** the three
flags cleared by one chained assignment `restored = any_click = hovered = false;` (BL zeroed first, both slots
stored from it; the order of the chain sets the slots: any_click at esp+0x10); in the window loop `win = prev;`
before the click-flag check (the binary's `mov ecx, esi` before `je`); the restore stores in the binary's order
(left_pressed, right_pressed, right_released, restored, left_released, left_double, right_double); the drop count
added in place (`if (target->count + drag_amount <= target->capacity) ... target->count += drag_amount;`: the sum
lands in ESI). Same behaviour.

**D13903 (game/new_int_multi_update.cpp; m-1020b worker 15; closer only). version_check_run 0x410f40:** case -1
and the code after the switch written out twice (no `default:`) keeps the jump table (-1..2) and the zero in ESI
(LCS distance 100 -> 46 instructions). Still differs in tail merging: the binary shares the SkipVerify tail with
cases 1 / 2 and sends both -1 results to one tail at the end; a goto to one fail tail gets closer (42) but is not
kept, and one shared tail loses the zero register.

**D13904 (game/s3d.cpp; m-1020b worker 15). level_load_s3d 0x46e1b0 is exact:** the mesh loop through
`lobj *m = &level->meshes[i];`, the door-name loop through `s3d_door_name *dn = &level->door_names[i];` (set
`dn->door`, `dn->name`), and the memory delta through a local (`int used = mem_bytes_allocated - start_mem;
level_s3d_mem += used;`). Same behaviour.

**D13905 (vsdk/unnamed_http_0x55a8e0.cpp; m-1020b worker 15). http_get::send_get 0x55abd0 is exact** with the
header loop as `for (;;) { if (line == NULL) { state = HTTP_STATE_UNKNOWN_ERROR; fclose(file); return; } ... }`:
the end-of-headers error inside the loop is not cross-jumped into the final else (D10903's open point).

**D13906 (game/s3d.cpp; m-1020b worker 15). s3d_read_geometry 0x46e910 is exact** with the face indexed as
`g->faces[i]` throughout the normal loop (no face pointer local: the binary keeps the face address in the frame
and the vertex array in EBX).

**D13907 (vsdk/unnamed_os_0x530b90.cpp; m-1020b worker 15). os_wndproc 0x530ff0 is exact:** the handled
messages `break` to one `return 0;` after the switch, DefWindowProcA is the `default:`, and WM_TIMER,
WM_PALETTEISCHANGING, WM_PALETTECHANGED and WM_DISPLAYCHANGE are separate cases, each `break;`. With the four
labels on one body 8966 tests 0x310-0x311 as a range (`cmp 0x30f; ja; ... jb / ja`); separate labels give the
binary's split at 0x310 and the 0x311 / 0x113 / 0x30f compares. Same behaviour.

**D13908 (game/unnamed_ghost_rider_0x4b9e70.cpp; m-1020b worker 15). ghost_find_floor_below 0x4ba0d0 is exact:**
`start` set member by member (z copied as a dword), then the ray's require / exclude / flags words and _unk18,
then `ray.start = start`, then the direction. Found by a statement-order search (13 orders, 2 exact). Same
behaviour.

**D13909 (game/new_journal.cpp; m-1020b worker 15; closer only). journal_handle_click 0x41af80:** the
description-line clamp page first (`int lines = page; if (journal_desc_line_count < lines) lines = count;`) gives
the binary's `cmp edx, eax; jge`. Still differs: font_h is in EBP in the binary (the row counter in the frame),
ours keeps font_h in the frame; and the constant 2 (EBP vs EDI) in the thumb loop.

**D14000 (game/items.cpp; m-1020c worker 1). ring_format_info 0x44b7a0 is exact** (the corrected asm.mjs listing
showed the whole function): both speed lines as `if (speed != 0.0f) sprintf("Speed: %3.2f", 1 / speed); else
sprintf("Speed: 1.00")` (the binary falls into the division and shares one "1.00" sprintf between the armour and
weapon cases); the multiplayer skill remap as `switch (skill) { case 0x14: ...; case 0x15: ... }` (`sub eax, 0x14 /
je / dec eax / jne`); the resist line through locals (`n = RING_ROUND10(v); fmt = XSTR(299)` or the negative pair,
then one sprintf); the Charges and Effect format strings loaded into a `const char *fmt` before the sprintf (the
binary loads the localised string into ESI ahead of the arguments); the weapon requirement with two flags,
`comma` (set with a skill name, picks ", ") and `has` (EDX / DL and CL in the binary), and the ", " / "Skill Req: "
choice as an if / else of two strcat; the stat lines with a block-scoped `int v` per branch. Same behaviour (the
speed test `jne` on C3 sends NaN to "1.00" in both forms).

**D14001 (vsdk/unnamed_key_0x5093a0.cpp; m-1020c worker 1). The key queue and key tables are one object,
struct key_tables key_tab (0x25d94e0-0x25da947, 0x1468 bytes).** key_flush (0x509ab0) clears the five 256-entry
tables in one loop through one pointer with fixed offsets (`mov edx, 0x25d9540` then `[edx + 0x800]`, `[edx]`,
`[edx + 0x400]`, `[edx + 0xc00]`, `[edx + 0x1000]`, compared with 0x25d9940) and the queue with two pointers;
separate arrays give six rep stosd. key_mark (0x5093a0) stores the press timestamp before it loads the press count
and re-reads key_queue_tail after the queue word store (0x50952b), which 8966 does for fields of one object only
(as D10503). Extent: key_queue first, key_queue_tail last (key_last_released 0x25da948 and key_down_state stay
separate). Field and type names ours, the old names are macros; no header declares them (only this unit uses
them), so no header change. key_flush also clears key_down_state inside the table loop (8966 moves it out as the
rep stosd the binary has before the loop). Won: key_mark, key_flush, key_clear (unit 23 -> 26 of 26).

**D14002 (notes only; m-1020c worker 1). Closer, still differs:** ds_play 0x55d110: the Play flags as `int flags =
0; if ((char)looping) flags = DSBPLAY_LOOPING;` (the binary's `xor eax, eax / test cl, cl / je / mov eax, 1`; the
ternary gives setne) cut the listing to 18 differing lines; left: the binary addresses the channel as
`[esi + ds_channels + field]` with ESI = ch * 0x2c computed before ds_duplicate_buffer (as ds_play_streamed does
after its indexed stores), ours forms the pointer (`lea esi, ds_channels[ecx*4]`), 48 bytes shorter. Tried: the
pointer before / after the duplicate call, every access indexed (ch then stays in ESI and the offset is computed
after the call), indexed 3D block then the pointer. Tried without a match: items_parse_table's armour texture
switch (`t->armour_textures[heavy][hero] + n`, split / reordered case 2 / 5, no `tex = NULL`, a default case),
item_page_out_all (EBX / EBP: declaration orders, a type local, early `s`), serpent_illusion_start (one register
rotation from 0x496cc7: locals, if / else tail, else-if), pf_read_region (zero in EBP and i in EDI in the binary:
row / no-n / k = 0 forms), ds_stream_open (the binary keeps three separate `or eax, -1` epilogues; outer-if and
`== 0 return 0` forms give neg / sbb).

**D14003 (vsdk/skeleton.cpp; m-1020c worker 1). vim_read 0x543430 is exact:** the seven submesh sizes stored as
`mesh->submeshes[i].sizeN = sN` and re-read as `offset += mesh->submeshes[i].sizeN` (the binary re-loads them
from the record, `xor eax, eax / mov ax, [ecx + 0x24]`, and keeps the last size in AX instead of the frame: the
frame is 0x2a8, ours was 0x2ac with `sm->` everywhere), the block pointers through `vim_submesh *sm` (declared
after the size stores), and the bone-name loop as `for (i = 0; i < mesh->skin.num_ext; i++)` (the binary
compares with `[ebp + 0x44]` each pass). Same behaviour.

**D14004 (notes only; m-1020c worker 1). skeleton::read 0x544090, closer:** the old-version key count seeks as an
if / else of two calls (`push 1` before the branch, one merged call: 467 -> 17 differing lines), and the morph
loop counter block-scoped (`for (int i = 0; i < num_morphs; i++)`: the binary keeps it in rot_used's frame
slot). Its five root-epsilon tests are `(float)fabs(x) < 0.0001f` (D6300: `fcomp dword [0x581930]`). Left: with
that form the operand order of later SIB addresses and two loads flips ([edi+ecx] against the binary's
[ecx+edi] in the rotation-key loop), and it does so by the count of the float compares modulo 4 (0 or 4 casts:
no flip; 1 or 5: rotation loop; 3: morph loop; dead code, unused locals, extra casts elsewhere and string literals
do not count); and the else-branch trigger loop walks a pointer to `.name` where the binary's points at `.time`
(`[ebp - 4]` for the name; the eof-branch loop already does). Tried for the trigger loop: a record pointer, a
walking pointer, buf outside the loop, a ternary, the branches swapped, a block-scoped counter.

**D14005 (notes only; m-1020c worker 1). Tried without a match:** fx_rec4c::update 0x55b460 (same instructions; the
frame slots of the position-blend temporaries and the on_bone block's locals are packed differently, e.g. the
binary shares rq with the sum temporary at -276: q0 / q1 declaration places, rm / brot orders, rq as a temporary
or declared first); conversation_controller::camera_capture 0x4d8e10 (`#pragma inline_depth(0)` with a
`__forceinline` transform: VC6 ignores __forceinline at depth 0, the transform is called); state_title_enter
0x431e20 (a volatile read of game_flags into a local, `int f = *(volatile int *)&game_flags & 0x1c;` then
`game_flags = (prev == 14 ? 2 : 1) | f;`, leaves only `and eax, 0x1c` scheduled before the `xor ecx, ecx`; not
kept); serpent_anim_remap_in (the binary keeps ready in c's argument slot and two induction pointers).

## m-1020c worker 2 (D14100-D14108): gr_opengl, gr_font, inputbox, khosani lab, snow

**D14100 (vsdk/gr_opengl.cpp). gl_convert_a8_to_rgba8888 0x54d5c0 is exact with the texel indexed as
`src[y * bm_w + x]`.** 8966 strength-reduces the row product into the outer induction variable and then emits the
SIB operands as the binary (`[esi + ecx]`); a walking row pointer (`src += bm_w`) gives `[ecx + esi]`. The base /
index order of a `[reg + reg]` operand (D12303) can therefore come from the index form, not from the operand order of
the C expression. Same behaviour.

**D14101 (vsdk/gr_opengl.cpp). gr_opengl_tmapper 0x550080 is exact with per-vertex element pointers.** `vector3 *p =
&pts[i]; float *t = tc[i]; ubyte *c = colors[i];` at the top of the loop body (in that order; the binary's induction
registers and their increment order follow from them), and the texture coordinates as `t[2] = 0.0f; t[0] = (t[3] =
v->sw) * v->u * u_scale; t[1] = t[3] * (v_scale * v->v);` (the binary keeps sw on the FPU with `fst`). Same values
(the products are the binary's).

**D14102 (vsdk/gr_opengl.cpp). gr_opengl_apply_tint 0x54f9a0 is exact with an int sum.** `int sum = rgb[0] + rgb[2] +
rgb[1];` and `sum * gr_tint_r` etc.: 8966 converts the sum once (`fild`), keeps the float in the dead argument slot
with `fst`, and loads it before the tint in the later products. With a float local it loads the tint first. Same
behaviour.

**D14103 (vsdk/gr_font.cpp). gr_split_str 0x509130 is exact with two line-end blocks.** The '\n' case and the
overflow case each end the line themselves (FreeSpace's split_str shape, each with `continue`); 8966 merges them into
the binary's one block with END in EAX, and the frame becomes 0x220. The overflow test is `if (w >= max_w)` with the
break-point case (`last_break >= 0`) first. Same behaviour.

**D14104 (vsdk/unnamed_inputbox_0x5521a0.cpp). ui_inputbox::draw 0x5530b0 is exact with the tail measured as
`&buf[len - n]`** (n counting up) instead of a pointer walking down: 8966 strength-reduces the index into the
binary's pointer and register choice. Same behaviour.

**D14105 (vsdk/gr_opengl.cpp). The resampling converters read the texel through a pointer to it.** `ushort *sp =
&s[(v / ONE) * bm_w + u / ONE];` and every channel from `*sp` gives the binary's `lea eax, [ecx + 2*eax]` and
`mov cx, [eax]` (D11200's open point): gl_convert_4444_to_rgba8888 0x54d8d0 and gl_convert_1555_to_rgba8888_scaled
0x54dc20 exact. gl_convert_1555_to_rgba8888 0x54da80 has the binary's load-then-dead-lea with `ushort p = s[..];
ushort *sp = &s[..];` and a / b from `*sp`, g / r from p (closer, 103 -> 26 differing lines; the alpha computation is
scheduled later than in the binary). Same behaviour.

**D14106 (game/level_script_khosani_lab.cpp). ls_khosanilab_puzzle_corner_pp 0x4f8100 is exact** (D12100's open
point) with the first y sum written `float ty = t.y; v2_set(out, out->x + t.x, ty + out->y);`: the float local makes
8966 load t.y first. Same behaviour.

**D14107 (notes; closer forms kept, behaviour identical).** gr_font_create 0x508560: both format switches on
`(unsigned)f->format` (the binary's compare order 0xF, 0x0F0F0F0F, -0x10 is unsigned ascending), `int y = 0; int x =
0;`, the PAL8 nibbles as `(unsigned char)((color >> k) & 0xf)` (byte AND then 16-bit movzx): 159 -> 43 differing
lines; left: the ALPHA4 loop keeps src in ECX (no entry jump) and does not turn `(g >> 4) << 12` into a `mov dh`.
ls_khosanilab_puzzle_trace_beam 0x4f7b50: `int start_dir = dir;` after the first turn_dirs (the binary loads dir into
EBP there), back_dir block-local (frame 0x48 as the binary: it reuses start's argument slot), `next = cur; next.x +=
d.x; next.y += d.y;` (the binary's dead store of cur.x into next.x): 272 -> 100; left: the shared tail of the a / c
turns is kept in case a by the binary, in case c by 8966 (goto, if / else and duplicated forms tried). snow_start
0x494ab0: a walking `snow_cell **cp` as the outer loop test and a next-row pointer: one swapped pair left
(`mov ebx, 15` / `add ebp, 0xb4`).

**D14108 (notes only). Tried without a match.** gl_convert_1555_to_rgba8888 alpha forms (eight); gr_opengl_read_
backbuffer (36 swap orders, index forms: the binary's +2-biased row pointers come with `buf[(i * w + j) * 4 + k]`,
9 differing lines left, all store scheduling); gr_opengl_bitmap's first corner (statement orders, pointer, declaration
orders); gr_opengl_set_mode (EBX / EBP swap); ui_inputbox::set_valid_chars / set_invalid_chars (function-scope `char
lo, hi` gives the 0x8c frame, but 8966 cross-jumps the three range blocks where the binary keeps them); state_game_
frame (the binary copies camera_fov and the frame times through the shared frame+4 slot with integer moves: float /
struct / array / volatile / assignment-expression locals tried); ai_state::goal_do_travel (the keep flag stored in
goal_flags' slot: bool / volatile / inline-helper forms; volatile is closest, 47 -> 17, not kept);
ai_state::pick_attack_target (single exit with goto: the binary's -1 in EBP stays); tiger_anim_remap_in (ready in c's
argument slot: ready / i declaration positions and the ternary; the serpent, phoenix and ghost copies differ the same
way); gl_build_mipmaps (heavily strength-reduced filter loops, not attempted beyond reading them).

## m-1020c worker 3 (D14200-D14206): boss AI, IRC chat tracker, net reliable sockets, script actions, shopkeepers

**D14200 (game/boss_ai.cpp; m-1020c worker 3). boss_ai_parse_table 0x4a1790 is exact** with the record pointer
and the next one as two variables: `next = table[--stage]; do { a = next++; ... } while (...)`. The binary keeps
`a` in ESI and the next record in EDI (`mov esi, edi; add edi, 0x20` before the name is parsed) and the table
argument stays in its slot; `table[--stage]` gives the binary's `lea edi, [esi + 4*esi - 5]; shl edi, 6` (with
`table[stage - 1]` 8966 folds the -1 into the final lea, `&table[...][0]`, `table[0] + ...` and casts change
nothing). Found with the corrected asm.mjs listing (the old one dropped the prologue). Same behaviour.

**D14201 (game/boss_ai.cpp; m-1020c worker 3). boss_target_list_build 0x4a2f60 is exact** with `n = 0` set in
each branch (the binary clears it after the single-player test) and the candidate written as `list[n].field`
(no `list++`): the frame drops to the binary's 0x24 bytes, the members pointer then lands in the dead `list`
argument slot as in the binary (D8903's open point). `boss_target *o = &list[n];` matches too. Same behaviour.

**D14202 (game/multi_chattracker.cpp; m-1020c worker 3). irc_add_user 0x4adb40 is exact** with the queue call
and `return 1` written in both the empty-list path and the append path. 8966 then cross-jumps the two
identical tails (the shared `next = NULL` store onwards), and malloc's argument pop stays deferred into the
tail's `add esp, 0x10`; it also stops shrink-wrapping ESI / EDI (D8701: the region now has several exits).
Rule seen here: an `add esp` that merges the pops of calls on two paths, after a `jmp` into shared code, means
the source repeats the tail on both paths. Same behaviour.

**D14203 (game/multi_chattracker.cpp; m-1020c worker 3). irc_process_input 0x4adcd0 is exact** with "list" and
the quiet raw command written as `if (list) { ... } else if (quiet) { ... } else return XSTR(...);` falling
through to the function's final `return NULL`, and the channel message as `else if (irc_cur_channel[0])`. With a
`return NULL` in each case 8966 merges xyz's copy into list's; falling through, list's copy jumps back into
xyz's as in the binary (the quiet copy stays partly separate). Same behaviour.

**D14204 (vsdk/unnamed_net_0x517590.cpp; m-1020c worker 3). From nw_accept_reliable 0x518ce0 to the end of the
object the code is compiled as with /Ow**, written as `#pragma optimize("w", on)` (guarded by _MSC_VER) before
0x518ce0. nw_accept_reliable, nw_NewSendReliable 0x518d70 and nw_ConnectToServer 0x518f30 then match (the
struct copy / store scheduling of D11803 and D8904), "a" (/Oa) gives the same three, and no other function of
the unit changes: the region can start anywhere after nw_buffer_push 0x517780 (from 0x517590 it loses
nw_buffer_push, as the whole-file /Ow of D8904). The unit's flags stay /O2. Whether the original file had two
parts or a pragma is not known; the pragma is a device, like the volatile stores of earlier rounds. No other unit of this worker
(script_internal, level_scripts_common, boss_ai, s3d, multi_chattracker, character, worldmap, party inventory,
evil_joseph) gains anything from "w" or "a" per function.

**D14205 (m-1020c worker 3; closer, kept, same behaviour).**
- script_action_walk_to 0x4dc6e0: `walk = false; if (type == WALK_TO) walk = true;` (the binary's two byte
  stores) and the move-through test as `if (path != NULL) { flags |= 2; return true; } return false;` (shares
  the fail tail): 90 -> 38 differing instructions, same size; left: the binary keeps `so` in EBX for the whole
  function, `np` in the frame and `a` in the dead `so` slot (declaration orders, `a[-1]`, so->actions reads
  tried).
- script_action_replace_weapon 0x4dccb0: the records written as `cutscene_saved_equipment[count].field` (no
  record pointer: the binary indexes the array with the count in a register, `shl ecx, 2`, and keeps old1 in
  EAX): 65 -> 3 differing lines; left: c in ESI and the count / `a` in EDI where 8966 swaps them (12 forms of
  the loop, declarations and initialisers tried).
- nw_NewCloseSocket 0x5191c0: the body inside `if ((unsigned)*socket < NW_MAX_RELIABLE_SOCKETS) { ... if
  (nw_socket) { ... } } else sprintf(...)`: the binary's layout (the bad-id message at the end, sharing the
  epilogue with the no-socket exit); left: the socket pointer in EBX from the entry (ours EDI) and the i * 2
  counter register.
- level_parse_shopkeeper 0x4cc4d0: the duplicate scans `goto next` (the binary jumps from the match straight to
  the `$End` test, no `i < n` test after the loop), the unknown item as an early `{ sprintf; continue; }`, the
  multiplayer price clamp `<= 1` (`jg`): 88 -> 65; left: the binary places the unknown-item block after the
  sell scan and indexes item_types with EAX.

**D14206 (notes only; m-1020c worker 3). Tried without a match:** boss_threat_add (inline fill helper, `== 0`,
float local; the `test edi` placement, D11704), level_trigger_point_left_of_line (not retried), ls_worldmap1_
pick_encounter_stage9 (goto / switch / early-return forms of the desert-night placement), evil_joseph_pick_
helper_summon (t + boss order, dz first, vector difference), script_action_combat_mode (all 24 declaration
orders, no `on` local: the binary keeps `on` in the dead `a` slot and skip in AL), script_action_replace_armor
(direct indexing, a bool helper: the early-out block order differs), ca_character::add_skeleton (block scope,
goto / result-variable single exit: the binary's epilogue belongs to the main path, the dup return jumps to it),
nw_ras_init (goto into the first fail block: moves it out of line; the loop failures jump to the first
FreeLibrary block in the binary, to the count < 1 block in ours), report_memory and s3d_read_door (not retried:
D11804 / D7702 tried them at length).

**D14300 (m-1020c worker 4; vsdk/ui/ui_gadgets.h, HEADER). The list box's entry head is a plain ui_listbox_entry;
ui_listbox() list_inits it in its body. Supersedes D4201's ui_listbox_entry_head.** The unit is built /Ob0, so the
head's inline constructor was called out of line; Sum.exe calls ui_listbox_entry() (0x52d540) for +0x2d4 and stores
next / prev (+0x2dc / +0x2e0) from the constructor body, which 8966 schedules ahead of the vtable store
(0x52d606-0x52d612). `list_init(&entries);` as the first body statement gives ui_listbox::ui_listbox (0x52d5c0) a
byte match. ui_listbox_entry_head had no other user; a full strict sweep loses nothing.

**D14301 (m-1020c worker 4; notes only). ui_listbox::add_entry (0x52da40) appends with list_append.** The binary
keeps the old tail in EAX for `->next = e` (0x52daf0-); the macro gives that. Still differs: the max_content_h loop
reloads content_height / max_content_h at its test (an unrotated loop); while, for (;;), goto and `+=` forms
do not reproduce it.

**D14302 (m-1020c worker 4; vsdk/gr_glide.cpp). gr_glide_create_texture (0x5492f0) byte match:** `int x = 0;` before
`sh`, the width clamp as `if (w - x > 256) sec->w = 256; else sec->w = w - x;` (the binary's two stores) and
`x += sec->w;` before `sec->h = sh;` (EDX / ECX roles at 0x549444-0x549456).

**D14303 (m-1020c worker 4; vsdk/gr_glide.cpp). gr_glide_create_mipmapped_texture (0x547cc0) byte match.** The
1555 / 565 / 4444 copies and the palette lookup store through `ushort *d` with `*d++` (the binary's running
pointer); the 8888 nibbles are ushort locals (`movzx dx, dl`, so 8966 does not fold `(x >> 4) << 4` into an AND);
the transparent-green zeroing is written b, g, r, a (888) and b, g, r, a (palette), which gives the register roles;
the LOD switches list the cases ascending (case blocks in source order, 1 first); large_lod / small_lod are
initialised before bm_unlock (EBX / EBP set before the call, 0x548107); the section fields are stored through
entry->sections-> each time (one load, the binary's store order), the local `sec` taken afterwards.
Also gr_glide_bitmap (0x54a800, still differs, frame now 0x144): bw / bh scoped to the one-section block, the
screen corners as `(float)(offset + x)` expressions, the scale divisors through float locals (fild / fild /
fdivp); left: vertex-store scheduling. glide_record::alloc (0x549b50, still differs): `index = used_tail =
used_head;` (used_tail stored first) and `addr + size` in the loop test (the add after the first next >= 0 test);
left: the b->next / b->addr and texture store order at the end.

**D14304 (m-1020c worker 4; game/characterinfo.cpp). character_can_use_summon (0x446e90) byte match.** The ring
checks are one else-if chain over the creature names with a single `return false` after it; each ring test is
`if (cond) return true;` only. A failed test then jumps to the shared false block (0x447396) as in the binary;
8966 turns each `if (x) return true; return false;` into setne.

**D14305 (m-1020c worker 4; game/characterinfo.cpp, include/engine/characterinfo/characterinfo.h, HEADER).
characterinfo_parse_table (0x4452f0) byte match.** (1) default_footstep_sounds is two arrays,
default_footstep_sounds_left[5] (0x59602c) and _right[5] (0x596040): the default-footstep loop indexes both from
their own absolute bases with one byte index (0x445ef1 `[esi+0x59602c]`, 0x445f17 `[esi+0x596040]`, `cmp esi,
0x14`); one [2][5] array gives 8966's single biased pointer. Same bytes in .data; only characterinfo.cpp uses them.
(2) `*(int *)info->default_weapons = -1;` (one dword store, no address in ECX; memset forms keep the lea).
(3) `m->next = info->moves[type];` before `m->category = type;` (the load ahead of the store, 0x446112).
(4) info->moves cleared by a `for` loop of NULL stores: 8966 turns it into the rep stosd with the lea first
(0x445f76); memset with any argument form puts the lea last. (5) The trigger time scale is 0.03333333f
(0x3d088888, one ulp below 1/30; `1.0f / 30.0f` is 0x3d088889).

**D14306 (m-1020c worker 4; game/ai.cpp). ai_update_perception (0x472e80) byte match.** No `goto visible`: the
obj_flags & 2 block writes `if (f & 0x40) { c->ai.flags = f & ~0x40; c->frametime = reported_frametime; }
return false;` and the end writes `if (culled == true) { c->ai.flags &= ~8; return true; } c->ai.flags &= ~0x40;
c->frametime = ...; return false;`. 8966 then cross-jumps the end into the first copy (0x4730a8 jne 0x472f5f) and
the register roles (game_flags in CL, f in EAX) follow. Same in evil_urath_ai_select_action (0x4c4db0, still
differs): the leap written out in each HP band instead of `goto leap` gives the binary's shared ladder tail
(0x4c4eb6 / 0x4c4ee1 jmp 0x4c4efa); left: the first `return 0` is merged into the later copy in Sum.exe
(0x4c4e64 je 0x4c4f7b), into the earlier one by 8966 (else forms tried).

**D14307 (m-1020c worker 4; notes only). Tried without a match:** the anim-size sum of alloc_level_buffers (the
load order does not depend on the names either: four renaming schemes give the same order); pxochat_page_down
(6 forms); pxochat_handle_click (the unsigned shift `(unsigned)d->flags >> 2` with the active flag on the left, as
gamelist, fixes `shr` and the compare order; left: -1 kept in EBX in the binary, 2 in ours, and the shared
cursor-blink tail); snap_scroll_bottom (6 forms), wrap_text (len local), set_vscroll_fraction (4 loop forms),
load_section (120 tail store orders, chained assignments), learn_chain_skill (cost block scope), can't get the
binary's late `push ebp`; cast_effect_track_add (pointer register: `cmp ecx, imm32` is the binary's extra byte);
mp_player_file_read_summary (6 more flag forms); machival_ai_select_action keeps `target` in memory in Sum.exe.

**D14400 (game/new_multichat.cpp; m-1020c worker 5). chat_draw_lines 0x41e900 exact: one gr_set_color_ptr call per
case.** The colour switch is `case 1: case 3:` text / `case 4:` magenta / `default:` text, each case calling
gr_set_color_ptr itself. 8966 cross-jumps the calls into the binary's `push colour; jmp call` shape, whose split
stack cleanup keeps the later gr_string calls apart; a colour pointer local with one call lets 8966 merge the
three gr_string calls instead. Same behaviour.

**D14401 (game/new_multichat.cpp). chat_draw_input 0x41ebd0 exact: the scroll offset reuses x.** x has its address
taken (chat_get_origin), so the binary's store of the offset to x's frame slot shows the reuse; the clamp is
`x &= (x > 0) - 1` (setg / dec / and) and the cursor choice an if / else of two gr_string calls. Same behaviour.

**D14402 (game/new_multichat.cpp). chat_input_add_char_wrap 0x41e6b0 exact: nested if / else, normal case first.**
The binary lays the blocks out as `if (c && len >= 63) { if (strlen(trimmed)) { if (/msg) { if (name) { if
(rest) ... else ... } else ... } else ... } else ... } else ...`; early returns put the short blocks inline. The
first strtok token goes through a local so it is evaluated before the XSTR load. Same behaviour.

**D14403 (game/living_entity.cpp). scale_stats_to_level 0x44ddb0 exact: `(float)level / ((float)info->level)`.**
In /Od code 8966 folds a bare cast divisor into `fidiv m`; the cast in its own parentheses gives the binary's
`fild m; fdivp` (checked on probe functions: double casts, int / long / unsigned casts and helpers do not).

**D14404 (game/spells.cpp; closer only). spells_parse_table 0x47a190:** the record indexed as `spells[i]` at each
use (157 -> 48 differing lines) and `prev_class = NUM_SPELL_GROUPS` assigned after the pool reset. Left: EBX / EDI
swapped for cls and the record pointer (declaration placement and the class store forms do not move it).

**D14405 (vsdk/unnamed_vif_0x5329f0.cpp). vif_instantiate_level 0x5337a0 exact.** `total = desc->count +
vif_particle_used` before `used` is read again; the free-list append indexes `r->particles[i]` at each use (a `p`
pointer local lets 8966 hoist the `p->next` store); the free-ring unlink loads prev / next into locals and clears
r->next first, with the one-record case as the else branch of `r->next != r`. Same behaviour (nothing writes the
global or the ring between the reads).

**D14406 (game/levelrender.cpp; closer only). levelrender_face_vertex_colors 0x48f8c0:** each byte of the tint sum
cast in its own parentheses gives the binary's `fild; fild; faddp` instead of fiadd. Left: the vertex loop's
register and frame layout (frame 0x20 vs 0x1c).

**D14407 (game/living_entity.cpp). save_state 0x457150 and load_state 0x458800 exact: the bool macro shapes.**
In /Od, `bool b = (x == 1)` is `sub / neg / sbb / inc` and `(b != 0)` is `neg / sbb / neg`; `? true : false` gives
the binary's `cmp / sete` and `test / setne` (probe functions). LE_PUT_BOOL / LE_GET_BOOL keep their do-while(0);
LE_PUT_SCRIPT / LE_GET_SCRIPT have none (the binary's loop-back jumps go to the bool statement's start). With the
macro temporaries changed, the block locals were re-laid out by the D6101 hash: save_state case 4 `int id; int n;`,
case 7 `int id; Player *play; int n;` (play, bucket 14 like n and id); load_state case 7 `ubyte eid; Player *pla;
int fx; item_info *old_a; item_info *prev_b;` (pla bucket 0, prev_b bucket 7). Same behaviour.

**D14408 (notes only; m-1020c worker 5). Tried without a match.** options_init (8966 forwards Summoner_options' first
dword past the rep movsd in every probe form: pointer, memcpy, helper, cast; only a volatile mouse_mode store moves
it, not kept), spell_roll_amount (`for (i = 0; i <= count - 1; i++)` gives the `dec / lea esi, [eax+1]` trip count
but with the test after the dec; 18 more loop / guard forms), spell_target_value (which return blocks are shared:
default inside the switch, result variable, polarity flips), Character::say (loop tail scheduling: 8 forms),
options_load_bitmaps (ESI / EDI for i: 5 forms), mp_handle_combat_sequence (the binary converts variant / skill to
ints in EBX / EBP early; ubyte special, int locals tried), projector_record::to_lobj_space (an inline
plane::transform reproduces the vector3 copy constructor call but expands plane::operator=, which the binary calls
out of line; inline_depth 1-4 no change), chat_input_handle_key (case order, fall-through, goto: the block order
does not move), vif_record_get_bounds (temporary slot: 6 forms), vif_draw_particles (120 declaration orders),
options_drag_sliders (width copy forms), summon_create (8966 merges identical get_int branches in every probe form:
else-if, switch, goto, helper), the /Od move_frame / update_head_look / ai_move_frame / clip_move_to_navmesh /
get_hit_by_spell (code differs as well as the frame: out-of-line operator calls inside expansions).

**D14500 (game/unnamed_frame_0x4323e0.cpp; m-1020c worker 6). game_is_paused 0x432da0 is exact** as
`if (game_flags & 4) return true; return false;`: 8966 turns the if-form into the binary's `shr al, 2 / and al, 1`;
`(game_flags >> 2) & 1` gives a 32-bit `sar`, `!= 0` a test / setne. Same behaviour.

**D14501 (game/unnamed_giant_salamanka_0x4c8030.cpp; m-1020c worker 6). ai_state::giant_salamanka_ai_select_action
0x4c87e0 is exact** with: `count = 0` after the busy test; each candidate written through `cand_t *e =
&cand[count++]` (handle, then score); the party-entity duplicate test leaving the inner loop by `goto next` (the
count-5 test after the store); the weighted pick as `pick = -1; ... { pick = i; break; } if (pick == -1) pick = 0;`;
`pt->pos = pos + owner->centre_offset` (vector3 sum, one temporary); the target offset as `vector3 d = t->pos +
t->centre_offset - gsal.offense_points[m->offense_point].pos` (dx = d.x, dz = d.z) before `pt` is taken; and the
success tail as `if (pick >= 0) { ... return ...; }` before the shared `none:` label. Same behaviour.

**D14502 (game/new_saveload.cpp; m-1020c worker 6). saveload_draw_info 0x426d10 is exact** with `char ampm[3]` and
`char buf[260]`: the binary's frame 0x25c and its slot order (ampm below w; 4 bytes between buf and line) are those
sizes. D13001's two strcpy calls kept. The buffers only grow; same behaviour.

**D14503 (game/new_saveload.cpp; m-1020c worker 6). saveload_scan_saves 0x4258c0 is exact** with the entry written
as `saveload_entries[saveload_num_saves].field` (no entry pointer: the binary keeps the index * 0x130 in EDX and
spills the filename address), and the list head reset as `list.prev = list.next = &list` (prev stored first).
With those, 8966 keeps saveload_num_saves in EBX across the loop latch as the binary does. Same behaviour.

**D14504 (game/new_automap.cpp; m-1020c worker 6). automap_label_text_input 0x403350 is exact** with one `if
(changed) automap_select_label(automap_selected_label);` after the key block instead of a call in the typed-key and
Backspace paths: VC6 threads both changing paths into the call (the Backspace path jumps to the label load, as in
the binary). Same behaviour.

**D14505 (game/unnamed_ai_groups_0x492b00.cpp; m-1020c worker 6). ai_group_collect_enemies 0x493ac0 is exact**
with: the member read as `ai_think_members[i]` at each use (no `m` local: the binary reloads `[esi]` after every
call); `vector3 dir; dir = e->pos - ai_think_members[i]->pos;` (assignment from the difference temporary); the cone
dot through `const vector3 &f = ...->orient.fvec` written `dir.x * f.x + dir.z * f.z + dir.y * f.y` (found by a
search of the six orders, three exact; 8966 emits y, z, x); a hit leaving the member loop by `goto found` with
`continue;` before the label; `fabsf` for the height test (float 3.0 constant). Same behaviour.

**D14506 (game/unnamed_ai_groups_0x492b00.cpp; m-1020c worker 6). ai_group_combat_think 0x492db0 is exact** with
the upkeep loop reading `ai_think_members[i]` at each use (frame 8, the pointer in ESI, -1 in EBP) and the AP
fraction as `int max_ap = get_max_stat(1); float a = ap; float b = max_ap; a / b > 0.4f` inside the if (the binary
stores both ints and loads them with fild / fild / fdivp; `(float)ap / (float)max` gives fidiv). Same behaviour.

**D14507 (notes only; m-1020c worker 6). Tried, still differing.** mp_session_reset 0x441490: a walking-pointer loop
gives the binary's registers but `jb` for its `jl`; the counter loop gives `jl` with ECX / EDX swapped.
mp_player_alloc 0x441610: `lea ecx, [eax+4]` (address of prev kept) not produced by any list_remove spelling.
mp_charselect_page_down 0x407410: list in EDX in the binary (four load orders tried). automap_center_on_player
0x4023b0: 8966 sinks x / y into the centred branch in every shape tried (four). automap_handle_click 0x402760:
`ui_button_state *b = &automap_buttons[i]` gives the binary's first loop (kept; first difference moves from +0x20 to
+0x52, then the label-box sums and the label loop's spilled counter). avoid_box_box_overlap 0x4a6f90: `vector2 d;
d = a->centre - b->centre;` gives a in EDX / b in ECX as in the binary, and `const vector2 &u = b->axis_a;` for k_ab
and k_aa their operand order; the remaining difference is the term order of the first two separating tests
(`k_bb * half_b + k_ab * half_a`); not kept (no match). saveload_handle_input 0x425de0: the binary places the row-hit
block inside the description-box code and shares the return-0 tails. avoid_area_rebuild 0x4a7cb0: goto-done,
if (0) and flag shapes keep the inline return. avoid_spline_point_get 0x4a77d0: ten more shapes (ternaries,
unsigned, nested, goto, pointer). ai_group_think_style1 / style2: register colouring (i / pointer / best);
localize_set_string: the binary keeps id in EBX and dest in the frame (four shapes).

**D14600 (game/gametarget.cpp; m-1020c worker 7). world_pick_lobj 0x43a770 is exact** with the four initialised
locals declared in the binary's store order: `int iter = 0; int index = -1; bool flag = false; float best =
10000.0f;` (8966 emits the initialisers in declaration order: iter, `or ebp, -1`, flag, best). Found by a
24-permutation search; one order matches. Same behaviour.

**D14601 (game/combat.cpp; m-1020c worker 7). living_combat::resolve_hit 0x467df0 is exact** with two changes that
only match together: `no_flinch` is an `int` (the binary keeps it in EDI: `xor edi, edi`, `mov edi, 1`, `test edi,
edi`), and the target is read as `opponent->owner` in place everywhere (no `Character *t` local; the binary
reloads [esi + 8] / [eax] after each call). Either change alone makes the code worse (frame and registers). Same
behaviour.

**D14602 (game/new_credits.cpp; m-1020c worker 7). credits_draw_lines 0x408ba0 is exact** with the alpha clamp
written `float_min(float_max(alpha, 0.0f), 1.0f)` (vsdk/math/vector.h's inline helpers) instead of the nested
ternary. The by-value argument puts alpha in memory (the dead `y` argument slot, [esp + 0x18]): the binary's
`mov [esp+0x18], 1.0` initial store after the compare, `fstp` / `fld` of the quotient, and the out-of-line
`fld 0.0; jmp` for the non-positive case. Same behaviour (the two forms differ only for a NaN, which the integer
quotients here cannot give). A pattern to look for: an in-memory clamp with the zero case placed after the
epilogue.

**D14603 (game/unnamed_shadows_0x4a37d0.cpp; m-1020c worker 7; closer only). shadow_project_face 0x4a46a0:**
`vector3 pr = d.rotated(...)` and `vector3 world = clipped[i].unrotated(l->orient) + l->pos` constructed in place
in the loop (the binary has no separate copies), and the alpha clamp as D14602
(`(float)sqrt(float_min(float_max(f, 0.0f), 1.0f)) * 255.0f`: the `fst` of f, its two reloads, the float 255
constant) give the binary's frame (0x1b8) and clamp. Still differs: 8966 biases the vertex pointer to &v->a and
keeps it in EBP (the binary: v in EDI, the clipped pointer in EBP); four index / block forms tried. Same
behaviour.

**D14604 (game/unnamed_shadows_0x4a37d0.cpp; m-1020c worker 7; closer only). lobj_raycast_draw_lists 0x4a4bb0:**
`first` declared in a block that starts after the three hit field stores puts it in the dead `hit` argument
slot as in the binary (frame 0xc, was 0x10; a block starting before the stores, or two separate `first`
variables, do not). Still differs only in an ESI / EDI swap (the binary: hit in EDI, the zero / counter / entry
in ESI); declaration orders (40), block-scoped `i` / `e`, a hit pointer local and a chained zero store tried.

**D14605 (vsdk/unnamed_floatmath_0x542500.cpp; m-1020c worker 7; closer only). float_format_buf 0x542830:** `int
n = 0;` declared before `char digit[2] = ""` makes 8966 store digit[1] from n's zero register (`mov [esp+0x13],
bl`) as the binary does. Still differs from 0x3f: the binary copies a into rem through memory before the `>= 1`
test and keeps the power and x on the FPU in the integer-digit loop.

**D14606 (notes only; m-1020c worker 7). Tried, still differs:** fast_inv_sqrt / fast_sqrt / fast_sqrt_refined
(the binary loads x into ECX and builds the result in EAX; 17 expression, local, pointer, union and flag forms
give EAX / EDX); mp_reset_net_stats (the binary's separate EDX zero for the seven counters: 19 orders, chains,
locals, volatile, inline helpers, loops); File::set_cipher_key (a local bit-field cast `struct { unsigned
encrypted : 1; }` on `file_slots[i].flags` with `int i = slot` gives the zero branch exactly and a later
`unsigned int k[3] = { key };` removes the key-test zero register, but the key branch still differs; not kept);
File::get_async_status (inline helpers do nothing under the unit's /Ob0); file_list_files (the frame order of
ext, the spilled path pointer and i: 24 declaration orders and 32 renames give the same layout);
mp_handle_actv_gm (an explicit &e->prev pointer for the unlink / append and two ping stores: no change);
character_can_talk_to (operand orders and component forms of a, b: 8966 canonicalises them);
pick_test_character (all d2 sum orders, the dx2 test forms); ai_choose_special (switch, separate tests, goto:
the push block still is not placed after the epilogue); get_protection (if / else inverted, goto layout, p6 from
e6). summoner_cd's three are the shipped no-CD patch (D2600), not compilable.

**D14700 (game/level_scripts.cpp; m-1020c worker 8). Save-buffer fields read with memcpy.** Where the binary
reads a save-buffer value into a register and tests it (`mov eax, [eax]; test eax, eax` instead of
`cmp [eax], 0`), or keeps a byte in a frame slot and reads it back as a dword with `and 0xff`, the value is
copied out with `memcpy(&v, p, sizeof v)` (the intrinsic: a plain load / store, but the local is address-taken).
Exact with it: has_conversation_data 0x4d0590 (the record count), hot_button_get 0x4d1770 (the kind byte, in the
dead `member` slot), read_inventory 0x4d2c90 (the closing size; with it `off += 4` comes before the size read as
in the binary), read_item_objects 0x4d31e0 (the lock byte). Same behaviour. Tried without result in
get_int, the speaker / topic bit functions, the level item lists and read_automap_labels.

**D14701 (vsdk/unnamed_checkbox_0x553ba0.cpp; m-1020c worker 8). ui_radio_button::create 0x5548a0 is exact**
as ui_checkbox::create (D11600): the width in a local before the height, the height as an if / else of box_h and
label_y + th (8966 then keeps label in EDI and puts tw in label's dead argument slot, th in the one frame local),
and the stores font, label_x, label_y in that order (found by trying all six orders). Same behaviour.

**D14702 (game/level_scripts.cpp; m-1020c worker 8). The table parsers' loops are top-tested whiles.**
script_filenames_parse_table 0x4d6100: `while (!tf.Accept("#End")) { ... while (tf.Accept("+Script:")) {
if (slot >= 7) break; ... } }` instead of `if (!Accept) do { } while (!Accept)` (that form duplicated the
$Level: head; the binary's dead `xor esi, esi` at the inner loop top appears only with the while), and every
save-info field through CONVERSATION_SAVE_INFO (also in the clearing loop, so its pointer is based at +4 as in
the binary). events_parse_table 0x4d39c0: the $Level: loop the same way, and the language name lowered through
an index (`for (i = 0; lang[i]; i++) lang[i] = tolower(lang[i]);`): 8966 strength-reduces it and creates the
pointer after the first-character test, the binary's order (a `char *p` loop sets it before). Same behaviour
(both rewrites are equivalent loop forms).

**D14703 (game/level_scripts.cpp; m-1020c worker 8). read_inventory 0x4d2c90: the item rename is written out in
the loop** (the four name hashes in a block, `hash` changed in place), not through savegame_remap_item_hash:
the inline function's parameter copy made 8966 keep gray in the frame instead of EBP. read_items 0x4d2b70 still
differs with every form (its index is strength-reduced into a frame slot; the binary keeps i in EBP and indexes
`[eax + 4*ebp + 8]`), so the helper stays for it.

**D14704 (game/level_scripts.cpp; m-1020c worker 8). read_item_objects 0x4d31e0: the three axes are temporaries
of the matrix constructor** (`matrix orient(vector3(1,0,0), vector3(0,1,0), vector3(0,0,1));`) and pos is
declared inside each loop: 8966 then gives pos the slot of the last-constructed axis, the binary's 0x5c frame
(named axes plus a function-scope pos needed 0x68). With D14700's lock byte the function is exact. Same behaviour.

**D14705 (game/level_scripts.cpp; m-1020c worker 8). `off += 4; return off <= size;` at the end of a writer.**
write_header 0x4d1df0 and write_living 0x4d2150 end `*(int *)(data + off) = off + 4; off += 4; return off <=
section.size;`. With `return off + 4 <= size` 8966 chose other registers for the compare (write_header) or kept
the constant 4 in EDI for every `off += 4` of the function (write_living). Both exact. Same behaviour.

**D14706 (notes only; m-1020c worker 8). Tried without a match.** hot_button_set / level_item_add (SIB base /
index order of the store; branch order, bool / char stores, unsigned offset), level_item_has / _remove /
read_level_items and get_speaker_bit (the binary creates the hash pointer after the count test and loads h into
a register: index forms strength-reduce but compare from memory; if / do-while forms; memcpy of h), get_int
(ECX / EDX swap; helper, explicit loop, local forms), read_items (strength reduction of the index), read_automap_labels
(closer: the label through a pointer `l = &automap_labels[index]` gives every instruction; left: count's frame slot
after x / z, declaration order, block scopes, int-typed x / z and memcpy reads do not move it), write_item_objects
(count / f slot order and the container pointer's base offset), layout_conversation (slot order of the two
strength-reduced pointers, the data / size store order), level_add_navpoint (the z fadd operand order),
two_stage_get / read_two_stage (the binary's hash spill and down-counting loop), cutscene_parse_table (/Od: the
register of `ent->num_fogs++`; for / pre-increment / += forms), mp_handle_swi_path (closer with a memcpy of walk:
only the `lea ecx` of `this` before the walk load differs; kept as before), ui_checkbox::draw (gr_line argument
scheduling), vfx_instance::init_nodes (this / node register swap), conversation::clear_speaker (needs a member
of conversation_speaker in conversation.h, D10904 (1); not done: no header change).

**D14800 (game/new_inventory.cpp; m-1020c worker 9). inventory_use_selected_item 0x416c10 is exact** (seen with
the corrected asm.mjs listing): the list branch is written like the equipped branch. The row index is a local
(`int row = inventory_selected_row + inventory_scroll_pos;`), `self_only` is tested first with its own use and
return, then `if (flags & 4) { begin; close } else if (flags & 2) { begin; close } else { use }` with no returns
inside the chain. `(flags & 4) || (flags & 2)` made 8966 fold the two tests into `test [mem], 6`; the chain gives
the binary's `mov ecx, flags; test cl, 4; jne; test cl, 2; je` with the begin and use blocks cross-jumped, and it
also stops the equipped branch's two begin blocks being cross-jumped (two copies, as in the binary). Same
behaviour.

**D14801 (game/new_inventory.cpp; m-1020c worker 9; closer only). inventory_handle_click 0x4159a0:** the button
loop through `const ui_button_def *d = &inventory_button_defs[i]; ui_button_state *b = &inventory_buttons[i];`
at the top of the body (skills_handle_click's shape) gives the binary's loop registers (ESI defs, EDI buttons,
EBX index, the loop ended on the defs pointer); LCS distance 277 -> 220. Still differs: the -1 constant is in
EBX instead of EBP, and the return tails / shared break target (0x415a32) are laid out differently.

**D14802 (game/unnamed_camera_0x4778b0.cpp; m-1020c worker 9). camera::find_occluders 0x478520 is exact:** the
second ray sets `index = 0; blocked = false;` before `ray.start = *a; ray.dir = *b - *a;` (the binary stores both
between the start's x / y and z stores), and the result is `return camera_num_occluders > 0 ||
camera_occluders_unk > 0;`. For that bool expression 8966 emits `xor eax, eax` / `mov eax, 1` as the binary does;
`if (..) return 1; return 0;` gives `xor al, al` / `mov al, 1`. Found by a 4 x 4 order / return-form search (two
of 16 exact, the other being `? 1 : 0`). Same behaviour.

**D14803 (m-1020c worker 9; notes only). Tried without a match** (forms in w9-NOTES.md): the max-0 AND register
of inventory_identify_item / destroy_selected_item / equip_item (34 more forms: ternary straight into the global
then the in-place `&=`, which gives the right AND but puts the min in the global's register; nested max(min());
`pos & -(pos > 0)`; unsigned); quaternion::exp's `fstp st(3)` (15 forms: casts, declaration order, ternary,
double, result local, `*this` copy); gr_setup_3d's threaded Glide test (if / else, ternary, blocks, a volatile
store or read); itemtrade_count_party_rows' `inc [mem]` (volatile increment, pointer, inline helpers, locals);
camera::auto_view_update's late `push ebx` (block-scoped near_enemy, three forms); gr_line's integer copies
(compound assignments); gr_get_frustum_planes' count register (six loop / declaration forms);
gr_project_vertex's reloaded flags at the end (five forms). gr_cull_bounding_box_transformed stays on D4502 (the
plane local's empty constructor is called under /Ob0; the binary has none).

**D14900 (game/unnamed_script_editor_0x4de0b0.cpp; m-1020c worker 10). script_write_objects 0x4e9ce0 now matches.**
The start-navpoint dispatch is one header condition, `(level == true && type == LEVEL) || (level == false && type
== CUTSCENE)`, and an `else if (level == false && type == LEVEL)` for the "+No create" record; the binary tests
the bool against 1 and then against 0 (`cmp cl, 1 / jne`, `test cl, cl / jne`), which only this shape gives. The
move-through warning has its own `msg[0x100]` between args[0x104] and name[0x104] (frame 0x30c). Same behaviour
for every bool value (a level that is neither 0 nor 1 cannot occur).

**D14901 (same file). The script editor reads its selection globals at each use.** script_editor_place_creature
0x4e58a0 (now exact) has no navpoint local: every use is `((navpoint *)script_editor_hover.object)`, reloaded
after each call as the binary does; the name-length test is signed (`(int)strlen(name) > 0x1f`, `jle`). The same
form brings script_editor_handle_input closer (the Pos / Ori drags read script_editor_sel.object directly, and
the Change button's character case indexes script_editor_cur_script->actions[index] again after get_name).
Same behaviour: no call in between changes those globals.

**D14902 (same file). script_editor_create_ui 0x4e0690 now matches (8788 bytes).** (1) The size-class radius is
`((vector3 *)script_editor_char_sizes[i])->len()` (the inline len loads x, y, z and squares them on the x87
stack, which a written sum never gives) and the height row is copied after the doubled radius (an integer
move). (2) The parameter rows loop over i alone, with y = 0x14 + i * 0xf and id = i * 2 + 4 computed from it:
8966 strength-reduces them into the binary's three induction variables and the `cmp edi, 0xe` test. (3) The open
min range box stores its callback before its font (statement order found by trying the six orders).

**D14903 (same file). script_editor_pick_hover 0x4e2bb0 now matches.** The character size row is copied as one
vector3 (`size = *(vector3 *)script_editor_char_sizes[s]`: three integer moves), and the two navpoint loops index
the table with `np = &navpoints[i]` in the body (the binary walks &np->pos and derives np as `lea ebx, [esi -
0x14]`), the opposite of D6005's walking-pointer form.

**D14904 (same file; closer only).** script_editor_handle_input: frame now right (752): ray / lhit declared after
the Y block of the Pos drag (the ori matrix temporary then shares lhit's slot), n in the Y block, the two vel
vectors block-scoped (they share the temporaries' slot). Remaining: register rotation in the right-click and
Change paths and one inline-budget difference (the binary calls vector3::operator= for the plane normal).
script_editor_add_navpoint: `lobj *o = lobj_from_index(hit.face)` before the second pick_layer call fixes the
call order; the level case's temporary slots still differ (D12502). script_editor_edit_button_cb: the field
loops wrapped in `if (n > 0)` after the switch (the binary tests n only on the wander path and falls into them):
same size now, the prologue pushes and one case's registers differ. script_editor_action_type_allowed: one-hand
as `return latched && species == 0` (EAX result) and the latched tests as `break` to a final `return false`:
every instruction matches, only the three `je` go to a different (identical) return-false block.
script_editor_level_item_ok: the name search restarts with `goto again` (the binary's `jmp` back to the count
load), 62 -> 22 differing lines. mp_levelselect_handle_click (game/new_levelselect.cpp): dx, dy, dz at function
scope give the 0x110 frame; register choice of list / rows / font_h remains.

**D14905 (notes only; m-1020c worker 10). Tried without result:** script_write_level_table (an 8-byte aligned
ebp frame appears whenever both the float compares and the double sprintf arguments are present; operator==, a
pointer, nested ifs, an early continue do not remove it), script_editor_default_action_navpoint (nine return /
loop forms), cmdline_take_argument (is_done before strcpy changes the whole register plan),
mp_levelselect_drag_scrollbar (expression and declaration orders), ui_button::draw_arrow (unsigned flags local
gives the shr but not the frame), select_prev (a prev local), focus_selection (inline budget, D10808),
script_write_level_script (two `sub esp, 8` scheduled around the fild of the door sound arguments; seven casts
and locals), the four lights functions (one fstp / lea each; no flag set fixes them), decals_draw_for_lobj (the
commutative fmul is sorted whatever the source order).

**D15000 (game/unnamed_worldmap_automap_0x42f1e0.cpp; m-1020c worker 11). automap_labels_save 0x430580 is
exact.** (1) The two open results go through a named local (`err = tf.Open(..); if (err != 0)`), which 8966
compares with the zero register (`cmp eax, edi`, as D11204). (2) The copy loop is `while (1)`, not `for (;;)`:
with `for (;;)` 8966 rotates the loop and duplicates the ReadLine / strncmp head; `while (1)` keeps one copy, as
the binary (0x430780). (3) The "#End" test is `if (strncmp(..) != 0) { write line; if (strcmp(..) == 0) {..;
break; } } else { header, blank, at_end = true; break; }`, so the matched-section block falls through and the
"#End" block follows it. The asm.mjs fix (first instructions of 90 functions) made this one readable. Same
behaviour (the first loop still does not test ReadLine's result).

**D15001 (same file). automap_labels_parse_table 0x42fd40 is exact** with the two level tests as separate early
returns (`if (cur_level_index == -1) return; if (!automap_corners_found) return;`): the shared TextFile
destructor exit is then placed right after the Open test, as in the binary; the `||` form puts it at the end.

**D15002 (vsdk/unnamed_gr_line_0x53e520.cpp). gr_bitmap_3d_oriented_quad_facing 0x53f060 is exact.** (1) `bw`
and `bh` are declared inside the `current_bitmap > -1` block: their slots are then free and 8966 puts the `-w`
temporary of the first corner there (the binary's frame 0x128; function-scope bw / bh give 0x12c). (2) The
divisions are `size * bh / (bw * 1.0f)`: the binary loads the divisor with `fild` and divides with `fdivp`;
`size * bh / bw` gives `fidiv`, and `(float)bw` / `(double)bw` casts are folded back into fidiv. A float local
for the divisor also gives fild / fdivp but its own frame slot. Same values (an int is exact in float, times 1).

**D15003 (same file). gr_bitmap_3d_projected 0x53eae0 is exact:** width / height are set in each branch of the
bitmap test (`if (bw < bh) { width = size; height = ..; } else if (bw > bh) { height = size; width = ..; } else
{ both size } } else { both size }`) instead of `width = height = size` before it, so 8966 keeps them on the FPU
stack as the binary; bw / bh block-scoped and the `* 1.0f` divisors as D15002; and `codes_and = 0xff;` comes
before `off = gr_scale_by_far_clip(size); depth = pnt->z;` (the binary saves EBX / EDI and sets BL before the
far-clip call, and stores depth after it). Same behaviour. gr_bitmap_3d_oriented_quad 0x53edd0 with the same
codes_and / depth order is down to one fstp scheduled before the integer stores of v[3] (9 positions of the
codes_and store, 5 orders of the v[3] statements tried; kept, closer).

**D15004 (game/unnamed_fx_0x489c40.cpp). fx_create_spell_area 0x48c630 is exact** with the network branch and
the enemy search as an if / else that both reach one `return fx->handle;` (D8701's one-exit rule): 8966 then
saves EBX / EBP / EDI after the `fx_new` NULL test, as the binary; with the early `return` inside the network
branch it saves them in the prologue.

**D15005 (vsdk/unnamed_gr_line_0x53e520.cpp). gr_poly 0x53e770 is exact:** the translucency scan is a block of
its own, `if (Use_buffered_alpha_polys) { for (i = 0; !translucent && i < n; i++) .. }`, and the begin test
repeats the global: `if (Use_buffered_alpha_polys && (translucent || alpha == 3 || alpha == 1)) begin(true) else
begin(false)`. The scan is then a one-exit region: 8966 saves EBP inside it (pushed after the test, popped right
after the loop), does not thread the loop's exit past the `test cl, cl` after it, and the two unbuffered begin
calls are one (the second Use_buffered test is threaded away). The nested if / else form gives a frame slot,
EBP in the prologue and three begin call sites. Same behaviour (nothing between the two tests writes the
global). Found in steps: while (1) / break forms of the scan give the loop body exactly but thread the exit.

**D15006 (same file). gr_line_3d 0x53e520 is exact:** local copies `gr_vertex *v0 = p0, *v1 = p1;` used
throughout and passed by address to gr_clip_line (the binary copies p0 into the frame and keeps v1 in p1's
slot), `clipped = false; ret = true;` assigned after the trivial reject (BL and the ret slot are set after
`push ebx`), and the clip failure `ret = false; goto done;` to the common `done: if (clipped)
gr_clip_free_temp_verts(); return ret;`. With one exit 8966 pushes EBX after the early return and duplicates
the free / return tail itself, as the binary. Same behaviour.

**D15007 (vsdk/unnamed_poly_clip_0x5519c0.cpp). polygon_clip_to_plane 0x551ca0 is exact:** `out[count] = ..;
count++;` (8966 strength-reduces out + count into a pointer kept in out's argument slot: the binary's `mov
[esp+0x64], eax` copy and `add ecx, 0xc`), the clip block entered by `goto clip` from the inside branch (`if
(inside) { out[count] = in[i]; count++; if (!next_inside) goto clip; } else if (next_inside) { clip: .. }`),
and `i++` written at the loop end before `next++` (`for (i = 0; i < n; )`), which places the in-pointer step
where the binary has it. Same behaviour.

**D15008 (game/unnamed_ai_combat_0x4bce60.cpp). ai_state::mode_combat_frame 0x4bff60 is exact:** every
`go_idle(); return;` is one block, `idle:`, written where the binary has it (after goals_remove_type1 in the
sleep / ice-coffin branch) and reached by `goto idle` from the hidden-target, inactive-target, inactive-owner,
goal-loop-end and no-target exits; the final no-target test as `if (t) { combat_approach(t); return; } goto
idle;` (the `if (!t) goto idle;` form makes 8966 duplicate the idle block there); the goal loop as `while (1) {
g = goal_find(..); if (g == -1) goto idle; goal_remove(g); }`; the aim point as `ai_combat_aim_point = bone_pos
+ t->centre_offset;` (the binary stores y and z through temporaries and shares the z store with the t->pos
branch). stance_done is then kept in BL. Same behaviour.

**D15009 (m-1020c worker 11; notes only). Closer, kept, not exact:** fx_missile_hit 0x48bac0 (the weapon tail as
`if (!fx->timer.elapsed()) { d = aim - start; d.normalize(); d *= frametime * 20; start += d; done = false; }
return done;` gives the binary's `mov bl, 1` and the d temporary: lcs diff 63 -> 41 lines; left: EBP / ESI
saved in the prologue instead of around the spell branch's stop / hit / process region, and vi / c in EBP / EBX
swapped; a done flag in the emitter loop, gotos and a split spell test were worse), shadow_create_blob_bitmap
0x489f10 (`int dx = 32 - x;` gives the binary's `fild dword` of a signed sum and the down-counting dx; left: y
kept in EBP and the pixel pointer in the frame, the binary the other way round: declaration orders, a yy local
and four loop shapes tried), ai_state::combat_cast_frame 0x4bfaa0 (the give-up blocks as inline copies instead
of a goto to one label: lcs diff 222 -> 126; the binary merges the 7 s and silence copies at the first site and
keeps the cur_character one at the end; left: the zero register (EBP against EBX) and a 4-byte frame
difference). gr_bitmap_3d_oriented_quad: D15003. ("lcs diff": instructions of a longest-common-subsequence
alignment of mnemonic + registers that differ.)

**D15010 (m-1020c worker 11; notes only). Tried without a match:** packfile_read_directory (p in pf's dead slot:
sector walked directly, a pf copy, outer / first declaration; as earlier rounds), choose_support_spell (EBX /
EBP swap: 5 declaration orders, the vitalize test as a default, a nested test, an `out` local,
`(*out)->flags`), worldmap_frame (the `fstp st(3)` sqrt: five more argument forms), summoner_cfg_load (the pxo
flag updates' registers: two statements, three operator orders, two bit-field casts; the rest is a register
rotation), status_handle_click (the list rectangle's arguments through locals give the binary's load order for
the outer test, lcs 135 -> 120, but the rows / font_h registers stay swapped; reverted), consider_life_drain
(the binary saves EDI in the prologue and walks the first loop from list, not list + 8: element pointers, a
walking pointer, a while loop, nested ifs; none moved it), combat_approach (the goal_do_cast return: five bool
forms), phoenix_anim_remap_in (`volatile int ready` puts it in the c slot as the binary, but 8966 folds the 3 / 0
store into neg / sbb / and; four test forms), approach_attack_slot (while (1): no change), polygon_clip_2d,
gr_poly_get_section, gr_draw_sphere, the phoenix AI and fx_frame / fx_update_missile / fx_find_spawn_point
(looked at only: register allocation and FPU scheduling throughout).

**D15100 (game/unnamed_ai_move_0x4b1410.cpp; m-1020c worker 12). The owner's centre through vector3::operator+.**
steer_jitter 0x4b2510: `owner->pos + owner->centre_offset + fvec * 2 + rvec * steer_offset` instead of
`owner->get_center() + ...`; the inline get_center's local c makes 8966 order the y / z additions the other way
round (9 bytes). Same value. Byte match (strict). The same form brought path_closest_point (`centre_offset + pos`,
nd 304 -> 98) and turn_towards (`d = *point - owner->pos + owner->centre_offset; d.y = 0`) closer.

**D15101 (game/unnamed_ai_move_0x4b1410.cpp). wall_avoid_steer 0x4b2890: no `Character *o` local.** With the owner
read through the member each time, 8966 pushes EDI only around the probe (`push edi` after the speed test, `pop edi`
before the hit test), as the binary; the centre as `centre_offset + pos` (operator+, copy-constructed), the side
vector as `path->pos - owner->pos - owner->centre_offset`, steer_side as an if / else of two immediate stores,
`!hit || rem * rem <= d2` as one test, one block from the probe to the end (the side vector's temporary then does
not reuse the centre's slot) and one exit (`goto done`). Same behaviour. Byte match (strict).

**D15102 (vsdk/effect_mem.cpp, vsdk/bmpman.cpp). Two exits inside the File block make 8966 push at the entry.**
The inverse of D8701: effect_base_read 0x51c8b0 (open / read failures return from inside the block) and
bm_prefer_vbm 0x502f70 (`if (bm < 0 && !f.find(..)) return;`) had their ESI / EDI pushes shrink-wrapped into the
block; with a second exit there 8966 pushes them at the entry, as the binary. Same behaviour. Both byte matches.

**D15103 (notes only). Closer, kept; tried, still differing.** mode_traveling_frame (cases in the binary's order,
TURN last; PATH and TURN set a `done` flag tested after the switch, so PATH jumps into TURN's test; the follow speed
as `float v = s < scale ? s : scale`; nd 842 -> 301; left: the wander point's y not constant-folded in the binary).
tentacle_beast_ai_select_action (no `t` local, `goto next` in the duplicate search, the squeeze points as copies
with y cleared; nd 523 -> 386; left: `this` / best slots and the count / pointer registers swapped). Tried, no
change: snd_set_volume family (local result, forceinline: /Ob0 keeps the call), eax_init, LineFile::detect_eol
(a flag-variable form gets the CRLF block's placement and `cr > cr + 1` test, not all at once), read_line,
rfx_preload (6 total forms), read_track SIB order, camera_blend_sequence::advance (slot order of the six angles is
independent of declaration order and names), tentacle_beast_boss_load, tentacle_beast_ai_frame (else-wrapped),
frame_hook (vector3 constructors called out of line in the binary), parse_arr_line (the inner loop's first exit
target), bm_set_reduced (`int n = e->num_levels` before the call: nd 63 -> 20, not applied), bm_lock_vbm,
bm_load_internal (`reduced` in BL), path_closest_point frame (0x58 vs 0x60; block forms).

## m-1020c worker 13 (D15200-D15206): vector, HUD, interface, memory report, skills

**D15200 (vsdk/unnamed_vector_0x523f30.cpp; m-1020c worker 13). Four vector functions exact, same behaviour.**
vector3::unrotated 0x5243d0: each column dot product as `x * r + (y * u + z * f)` (the binary adds z * f and
y * u first, then x * r; the 48 plain orders of D10101 all give z, x, y). vector3::length_quick 0x5240e0:
`(t + (t * 0.5f)) + a` (the parentheses keep 8966 from folding `t + t * 0.5f` into `t * 1.5f`) and a block-scoped
`float t` for each of the three swaps, which gives the binary's frame (a 0x0, c 0x4, t 0x8; declaration order and
names have no effect). vector2_closest_point_on_segment 0x525410: `float t = ap.x * ab.x; t = (t + ap.y * ab.y) /
ab.len_sq();` (the binary keeps the first product on the x87 stack after `fadd st, st(1)` and pops it after the
store; the product operands as ap * ab), then `t = float_max(t, 0.0f); t = float_min(t, 1.0f);` as two statements
(the binary finishes float_max before pushing float_min's 1.0f). vector2_dist_sq_to_line 0x5253a0: `float d = ap.x
* ab.x; return ap.len_sq() - (d + ap.y * ab.y) * (d + ap.y * ab.y) / ab.len_sq();` (8966 computes the sum once,
with the same leftover product). The three random direction functions still keep a copy of z on the x87 stack
(15 more forms tried: block scopes, parentheses, locals, z through r, sqrt twice).

**D15201 (game/new_hud.cpp; m-1020c worker 13). Three HUD functions exact through pointers and reused locals, same
behaviour.** hud_load_bitmaps 0x40cc10: `ui_button_state *b = &hud_buttons[i]; ui_button_def *d = &hud_button_defs[i];`
for the whole loop body (with only b the induction variables change; the binary indexes pos through d and ends the
loop on the button pointer). hud_update_portraits 0x40d160 (the listing start fixed in asm.mjs): the highlight
`flags &= ~4` through a block-scoped `ui_button_state *h`, and the clearing loop's two buttons through `b` and `h`
locals (the binary then stores the slot last). mp_draw_loading_status 0x4102a0: the panel width and height held in
w and h, the variables gr_get_string_size fills later (the binary's slots 0x0 / 0x4), instead of panel_h and a
reread of mp_loading_panel_width.

**D15202 (vsdk/memory.cpp; m-1020c worker 13). mem_report_line 0x512640 exact:** `int tl = strlen(text); for (i = 0;
i < 65 - tl - len; i++)` (the bound in the loop test gives `65 - tl` first; every `pad =` spelling, D13204, gives
`65 - len` first). Same behaviour (tl and len do not change in the loop).

**D15203 (game/new_skill.cpp; m-1020c worker 13; closer only). character_use_skill 0x429410:** the appraise test
needs no root variable: `user->get_skill(SKILL_APPRAISE) > (float)sqrt((double)type->value) * 0.0014084507f` and 8966
itself calls get_skill first and stores the root (`fst`) in the dead target slot. Next difference: the pick-lock
door test (binary `fld [door+0x58]; fcompp`, ours `fcomp [door+0x58]`; ge / not / parenthesised / local / swapped
branch forms give the same).

**D15204 (game/unnamed_interface_0x411270.cpp; m-1020c worker 13). ui_choice_popup_hit_test 0x413970 exact:** the row
height in h and the row width in w (the title size's variables; the binary keeps the height in the frame and reloads
it after point_in_rect), and the step as `y += h, i++`. Same behaviour. ui_choice_popup_show 0x4136a0 closer:
`max(w, ui_choice_popup_w)` gives the binary's `jle`; the registers of the two loads and the count load still differ.

**D15205 (game/new_hud.cpp; m-1020c worker 13; closer only). hud_handle_keys 0x40d930:** the cases reordered to the
binary's body order (number keys, C, H, I, M, Q, R, S, X, Tab, Enter, Esc, F1, Space / NumLock, F2-F9, F12,
Ctrl+L, Ctrl+S, Shift+Tab): the first difference moves from instruction 252 to 441. 8966 emits the compare-chain
cases (0x100f, 0x401f, 0x4026) last wherever they stand; the binary keeps the shared `game_play_sound(0x23) /
return 0` tail in the Esc case, 8966 in quicksave's (Esc last, chain cases before Esc, Esc with an else: no gain).

**D15206 (notes only; m-1020c worker 13). Tried without a match:** hud_drag_icon_begin (the binary reads the flags
word and ORs active in separately: 10 field orders, signed / narrower fields, a pointer; only the pointer form reads
the word, then masks each field), hud_hot_buttons_drag (dst loaded into ECX and copied to EBP: declaration orders,
inner scope), hud_portrait_get_color (the clamp through CL: 15 if / ternary / temporary forms, a function-scope v),
dialogue_frame (the tail: key loaded once before the mode test in the binary; else-if, ternary, early return,
result local), mp_handle_ai_disable and mp_handle_status_add (register choice; offset / kind / memcpy / obj forms),
vector3_angle_and_axis (the acos store is scheduled after two pushes in the binary; its slot is already a's),
gr_poly_clip_edge (base / index order of the uv addresses: pointer locals and operand orders), wav_open (imul order:
casts, locals, swap), TextFile::ParseInt (this / value registers: declaration orders, expression forms;
ParseString: a cursor pointer local, which also cost another parse.cpp match, not investigated),
Crc32::update (pointer and index forms), snd_stream_fill_pcm (declaration orders do not move the frame),
editor_dialog_handle_event (the event load is hoisted above the button compares in ours: break / goto / casts),
ui_message_box_show (the binary rereads the flags byte after storing the word: a volatile read gives most of it,
not kept), interface_init_global (the binary keeps screen_w on the x87 stack and base_w in the frame; if / swapped
declarations / initialiser), ui_3d_begin, ui_choice_popup_draw, hud_interact_nearest (the binary pushes EDI and EBP
only around the door and container loops).

**D15300 (game/unnamed_lobj_raycast_0x4ae890.cpp; m-1020c worker 14). lobj_raycast 0x4ae890 is exact:** one
local `vector3 d = ray->start - l->pos;` for the first rotated input, then `d = ray->dir + ray->start - l->pos;`
(assignment: the binary's temporary + copy, with the end point's x never stored) and the second result in a
named local, `vector3 r = d.rotated(l->orient); ray->local_dir = r - ray->local_start;` (the binary reads the
result from its frame slot, not through EAX). Frame 0x30: d at 0, the sum temporary at 0x0c, the first result /
the difference temporary at 0x18, r at 0x24. `dir + start` is `start + dir` (float addition commutes exactly).

**D15301 (vsdk/targautils.cpp; m-1020c worker 14; closer only). pcx_read_bitmap 0x555f60:** with `count`
declared without initialiser and `count = 0;` after `int pos = 0;` (c still initialised at the top) every read
and seek before the decode loop pushes the zero in EBP as the binary does (first difference 0xa6 -> 0x1a6).
Left: the binary clears pos (ESI) between the last read's `lea ecx, f` and the call, 8966 clears it before the
arguments and pushes ESI for the two zeros. Tried: pos before / after the read or the error test, pos at the top,
`int pos = count = 0`, `(pos = 0, f).read(...)`, count as the read's arguments, unsigned pos.

**D15302 (game/unnamed_lobj_raycast_0x4ae890.cpp; m-1020c worker 14). A centroid built with the vector3
constructor evaluates its arguments right to left (z first, all three kept on the FPU and stored at the end);
where the binary computes and stores x, then y, then z, write the components one by one** (`vector3 centre;
centre.x = ...; centre.y = ...; centre.z = ...;`). lobj_ray_test_faces 0x4aea50 is exact with it;
lobj_sphere_test_faces 0x4aec20 is closer (111 -> 91 differing lines; left: the binary's dead second
`t > 1` compare in the t < 0 path and its frame, 0x34 against 0x38; `two ifs`, else-if, nested, ternary,
bool, mirrored compares and a goto form tried for the in-range flag). Other units may have the same pattern.

**D15303 (vsdk/unnamed_bezier_0x558a30.cpp; m-1020c worker 14; closer only). spline::eval 0x558f40:** `i++`
right after `u = t - i` (the binary's `inc eax`, then 3j+6 / 3j / 3j+3), the x sum's two far points as
`points[i + 2].x` / `points[i + 1].x`, and the products with negative constants parenthesised
(`(-0.5f * u3) + u2`, `(1.5f * u3) + (-2.5f * u2)`, `(-1.5f * u3) + (u2 + u2)`): without the parentheses 8966
folds `-c * x` into a subtraction and shares 0.5 * u3 / 1.5 * u3 between the weights; with them it keeps the
binary's separate -0.5 / -2.5 / -1.5 multiplies. Now the same 357 bytes; left: the pops of EDI / ESI come before
the y sum's last add in the binary and before the z sum's in ours (an all-index form, a block-scoped p and a
const pointer tried). The values are the same (negation is exact).

**D15304 (vsdk/gr_direct3d.cpp, include/vsdk/gr/directx/gr_direct3d.h; m-1020c worker 14; header change).
gr_d3d_draw_primitive 0x538760 returns HRESULT and is exact.** The binary keeps EAX (DrawPrimitiveVB's result)
through the epilogue and does the `d3d_vertex_buffer_pos += count` in ECX, and the other branch ends in
`call [edx+0x64]; ret` with the call's EAX: a void function has no reason to spare EAX, so the declaration was
wrong. No caller reads the value (the six call sites in gr_direct3d.cpp are statements). The lock flags as
`if (pos + count >= 1000) pos = 0; if (pos == 0) discard else nooverwrite` give the binary's three loads and
jumps (the old else-if chain let 8966 preload one flag). Same behaviour. mingw over every .cpp and a full strict
sweep after the change.

**D15305 (m-1020c worker 14). Still differ, tried this round (do not repeat):** gamecreate_handle_click
(default case first, default goto, a label on the field test, `if (active) break; return 0;`, a tail switch on
KEY_ESC, `key != 0`: the field test stays after the default case, the binary places it after the key code);
pickup_draw_item_list (i / x / y declaration and assignment orders: x and i swap EBX / EBP);
gr_d3d_set_window (eight operand orders and clamp forms, the stored globals as operands: the lea base / index
order stays); ui_gadget::set_parent (separate head locals, a link pointer, an inline push-front helper: the else
branch's head stays in ECX, the binary's in EDX); ui_gadget::send_event (ternary, value copy, self pointer);
fx_recb0::get_fade (`/=`, alpha conversion forms: the fld is still scheduled above `sub esp, 8`);
m2v_skip_extension_and_user_data (8 case orders, continue forms: 8966 sorts the cases; the first vcall loads ECX
before EAX in the binary); dialogue_line::draw (six association orders of `y + scroll + top`, a helper, `*&top`:
8966 normalises the sum, the binary computes `y + scroll` with lea and adds top last); ix_point_in_polygon
(`alpha + beta`, early return, a sum local, nested forms: the binary loads beta and adds st(1), keeping alpha);
tga_rle_encode_row (42 orders of the initialisers); effect_stream_start (`return effect_stream_state`, the
assignment's value, `true`).

**D15306 (vsdk/unnamed_ix_0x53ac20.cpp; m-1020c worker 14; finding, not applied). seg2d_intersect 0x53d8f0:**
the binary has no frame: dx1, den, fx, fy and dy1 live in the slots of a, b, c.x, c.y and d.y. With the two
vector2 parameters by value 8966 treats c and d as whole objects (frame 8); with four float parameters it reuses
the slots as the binary does and the first difference moves from 0 to 0x38 (then: den kept on the FPU with
`fcom` / `fst`, and the second den test, which 8966 drops; with `if ((den = ...) == 0.0f)` and float
parameters the function is 221 bytes like the binary's, the FPU order still differs). The signature is not
changed (vector2 by value is what the caller pushes; the original may have had a plain struct or separate
floats).

**D15400 (vsdk/window.cpp; m-1020c worker 15). ui_window::send_event 0x52aff0 is exact** with no return statement
after `if (callback) return callback(event, value);`: VC6 warns C4715 and the function returns whatever EAX holds,
which on the NULL path is the tested callback pointer, so AL = 0 = false, exactly the binary's shared `ret 8`
(`je` to the `ret` after the call's `add esp, 8`). Same behaviour (the caller reads AL). The same shape may explain
ui_gadget::send_event (0x52c330) and other "return the callback's EAX or the NULL pointer" functions.

**D15401 (game/unnamed_pfgeom_0x4a4ee0.cpp; m-1020c worker 15). pf_first_edge_crossing 0x4a53d0 is exact** with the
squared distance written `dx = a.x - pt.x; dy = a.x - pt.x; d = dx * dx + dy * dy;` (the y line is a copy of the x
line, the game bug already noted). With `dx * dx + dx * dx` 8966 folds the sum (`fadd st, st`); with two variables
it keeps dx on the stack, spills the copy into the dead `hit` argument slot and multiplies it from memory, as the
binary does. Same behaviour. Reproduction device: a duplicated subexpression in its own local.

**D15402 (vsdk/unnamed_clip_0x5590b0.cpp; m-1020c worker 15). bbox_silhouette_corners 0x559890 is exact** with the
table row indexed in place (`bbox_silhouette_table[code][i]`, no row pointer) and the corner written to `&out[n]`
(no `out++`). Same behaviour (n == i while the loop runs).

**D15403 (game/new_int_multi_update.cpp; m-1020c worker 15; closer only). version_check_run 0x410f40:** the
download status as `switch (status) { case -1: ...; case 0: return 4; case 1: { ... } }` plus the error code after
the switch gives the binary's -1 / 0 / 1 compare chain, the inline unknown-status path (done through ESI, joining
the -1 tail) and the fetch error sharing case -1's block (75 -> 43 differing instructions). Still differs: the
binary sends cases 1 / 2 of the inner switch to the SkipVerify tail; goto, break-to-tail and case orders were
tried (a goto moves the constant 1 into EBX). Same behaviour.

**D15404 (game/unnamed_render_scene_0x481680.cpp; m-1020c worker 15; closer only). levelrender_add_effect_entry
0x4841e0:** `vector3 d = pos - levelrender_eye_pos;` and `e->depth = levelrender_eye_orient.fvec.dot(d);` right after
the type / fx stores give the binary's x87 order (fvec.x, fvec.z, fvec.y) and the stores' interleaving; only the
place of `pop esi` differs (36 store-order permutations tried). Same behaviour.

**D15405 (game/unnamed_render_scene_0x481680.cpp; m-1020c worker 15). levelrender_effect_apply_vis 0x483f90 is
exact** with the tag count in its own local declared at the tag loop (`int nt = base->num_tags;`) instead of
reusing n: 8966 then keeps the tag offset in the dead `fx` argument slot, as the binary does. Same behaviour. A
reproduction device for "dead argument slot" differences: split a reused variable.

**D15406 (game/unnamed_render_scene_0x481680.cpp; m-1020c worker 15; closer only). level_vis_parse 0x486d40:** the
first loop as `for (unsigned int i = 0; i < LEVEL_NUM_LAYERS; i++) level_layer_vis[i]...` gives the binary's
pointer biased to .sets (`[eax - 4]`, `jb`); a signed index gives `jl`. Still differs: the binary keeps 0 in EBP
(set in EBP in the inner loop, re-zeroed after it) and tf in EDI. level_vis_reset 0x487030 with the same index
loop also gets the bias but then 8966 puts the constant 1 in a register (seven forms tried, not kept).

**D15407 (vsdk/window.cpp; m-1020c worker 15; closer only). ui_window::handle_arrow_key 0x52abf0:** one
function-scope `int sw, sh;` used by both the selection branch and the gadget loops (the old gw / gh) gives the
binary's frame slots (both pairs at frame +0 / +4; 61 -> 54). Still differs: the register choice of the left-case
distance and of its get_size arguments (every expression order compiles the same).

## m-1020d worker 1 (D15500-D15504)

**D15500 (game/unnamed_avoidance_0x4a6710.cpp; m-1020d worker 1). avoid_sort_refs_along_segment 0x4a8aa0 is
exact** with the corner pointer as an if / else of two assignments (`if (s->side) p = &...corners[s->a].pos;
else p = &...corners[s->b].pos;`): each branch then has its own `movsx` of the short index, and the EBX / EBP roles
of use_x and the count follow. The ternary inside the subscript (and `int k` forms, ternary or if / else, and int
casts) convert after the join and keep the swapped registers.

**D15501 (game/new_automap.cpp; m-1020d worker 1). automap_center_on_player 0x4023b0 is exact** with the
player's map pixel as an int pair array (`int xy[2]`) instead of two int locals: 8966 sinks both conversions of
two scalar locals into the centred branch (D10203, every shape: goto, early return, inverted test, flag copy,
ternaries, half-size locals), but not stores to array elements. The binary converts both before the
automap_centered test, as with the array. Possibly worth trying where 8966 sinks a computation into the only
branch that uses it and the binary does not.

**D15502 (game/unnamed_avoidance_0x4a6710.cpp; m-1020d worker 1; closer only). avoid_box_box_overlap 0x4a6f90:**
`vector2 d; d = a->centre - b->centre;` (a in EDX, b in ECX, as w6 found) plus k_bb written out b-first and
k_ab / k_ba / k_aa as inline dots (`b->axis_a.dot(a->axis_b)`, `a->axis_a.dot(b->axis_b)`,
`a->axis_a.dot(b->axis_a)`, found by a search of all 256 spellings: written y-first / x-first, either dot
receiver) gives the binary's k slots (k_bb / k_ba in the dead a / b argument slots); byte-level instruction LCS
difference 38 -> 12. Left: the x product of k_ab (a first in ours) and the order of the two k terms in the first
two tests (k_bb / k_ba first in the binary). All 256 term / factor orders of the four test sums, the 24
declaration orders, `fabsf`, references and pointers for the axes compile the same or worse; a parenthesised term
moves the slots again. Same values (the products and sums are the same, added in the order the binary adds them).

**D15503 (game/unnamed_avoidance_0x4a6710.cpp; m-1020d worker 1; closer only). avoid_circle_contains_point
0x4a6bf0:** `r2 = (margin + ob->half_b) * (margin + ob->half_b);` after `r = margin + ob->half_b;` gives the
binary's `fld st(0) / fmulp` (8966 CSEs the sum into the stored r and squares the register copy); `r * r` reads r
back from memory. Left: r goes to p's dead argument slot in ours, to ob's in the binary (declaration orders, an
extra pointer local, d first or last: no change).

**D15504 (m-1020d worker 1; not reproduced).** ui_window::handle_arrow_key: the first difference is a jump
displacement (one byte shorter left-case distance code); all 10 spellings of each of the two distance sums
compile the same. ui_window::init: `this` in ESI and both registers pushed at entry in the binary; for-loops, win
at function scope, no `p` temporary, an else for the non-modal part, explicit this-> stores: no change.
cmd_avoidance: the help branch's jump is threaded past the doing_help test in ours (store order swapped, `== 1`
test: no). mp_read_spell_targets: req in EDI and walked as the loop pointer in the binary (no p local, net in the
loop block, declaration orders: no change). automap_center_on_player's sinking shapes (above).

**D15600 (game/pathfinding.cpp; m-1020d worker 2; behaviour fix in the decomp). pf_check_direct_walk 0x49e620:
the length that *max_len clips is |(to.x - from.x, to.y, to.z - from.z)|, not |to - from|.** After `end = *to`
(0x49e64b-0x49e65f) the binary loads end.y and stores it unchanged as d.y (0x49e663 / 0x49e667); there is no
`fsub [esi+4]`, while the x and z terms subtract from->x / from->z (0x49e657, 0x49e66f) and the clip branch does
load from->y (0x49e6c1, end.y = from->y + to->y * k, which nothing reads). The decomp wrote `d = end - *from`, so
it computed a different len, and clipped at a different point, whenever from->y != 0. Now written per component
(`d.y = end.y`). This is a decomp-fidelity fix (the binary does X, the decomp did Y), not a game-bug fix; whether the
original meant a height-free length is not knowable. Still differs: the binary copies *to into end with integer
moves and reloads it from the stack, ours forwards to's fields into the x87 stack (per-component, `vector3 end =
*to`, `vector3 end(*to)`, a from copy, `f.y = 0` and five statement orders tried).

**D15601 (game/pathfinding.cpp; m-1020d worker 2; closer only). pf_sweep_test_wall 0x49dc20:** the end points
through vector2's float constructor (`vector2 a(wall->a->x, wall->a->y)`, as pf_wall_normal), `dir = b - a`,
`rel = s->to - a` before `(s->from - a).dot(n)`, `off = 0.03f * n`, `p = s->from - off`, `q = s->to - off` and
`s->hit = c * (s->to - s->from) + s->from` give the binary's first 34 instructions exactly (313 -> 279 differing
lines). Still differs: the binary copies n = (dir.y, -dir.x) through the x87 stack (ours: an integer move; the
vector2 constructor, both store orders and `0.0f - dir.x` tried), keeps kind in EBX, and packs its temporaries
into a 0x48 frame, the argument slot included (ours 0x58). Same behaviour.

**D15602 (vsdk/unnamed_vif_0x5329f0.cpp; m-1020d worker 2). vif_draw_particles 0x533620 is exact** with the blend
mode as an if / else of two gr_bitmap_3d calls (`if (rec->vis_flags & VIF_ADDITIVE) gr_bitmap_3d(.., additive);
else gr_bitmap_3d(.., alpha);`) instead of a conditional in the argument: 8966 then loads the mode into ECX, the
size into EDX and &pt into EAX as the binary does (D12602's "register rotation"). A mode local, an if / else into a
local and both default-then-override forms were tried and do not match. Same behaviour.

**D15603 (vsdk/geom/vfx.h, vsdk/unnamed_vif_0x5329f0.cpp; m-1020d worker 2; header type, closer only).
vfx_inst21c::tag (+0x6a) is unsigned short.** Its only load in the tree, vif_emitter_update's
compute_bone_matrix(em->tag) at 0x533048, zero-extends (`xor eax, eax; mov ax, [edi+0x6a]`); `short` gave
`movsx`. The only other use is a 16-bit store (vfx 0x536950 unit), which compiles the same. All units compiled
(mingw and the strict VC6 sweep). vif_emitter_update also takes the force as `force = curve(..); force *=
reported_frametime;` (the binary keeps the curve value on the x87 stack and pops it after the store; one product
expression folds it), 354 -> 309 differing lines. Still differs: the frame slots of remaining / interval / force
(the binary: remaining at +0, interval +4, force +8, spawn +0xc in memory, i in EBX; 24 declaration orders, a
block-scoped spawn and a block-scoped counter change nothing; ESP frames do not order by name, D10802). Same
behaviour.

**D15604 (game/unnamed_tiger_0x499e50.cpp; m-1020d worker 2; closer only). ai_state::tiger_combat_update
0x49b6e0:** the binary has one go_idle call (0x49b9c2), reached by four jumps, with the epilogue shared with the
early `return` (0x49b9c9). Written as nested ifs (`if (m != NULL) { roar...; if (charge_state == 2) { if
(!follow_path(0)) { ...mount idle; return; } else { ...; return; } } } go_idle();`) the layout matches
(239 -> 220 differing lines; a goto version gives 224). Still differs: the binary keeps `fire` in BL (EBX
saved) and tests it at the join; 8966 threads the constant through and drops the variable (bool / int, top /
block declaration, the test's operand order and an assignment from the condition tried). Same behaviour.
tiger_ai_frame's only difference is the zero constant in EBP (D7006).

**D15700 (game/new_charselect.cpp; m-1020d worker 3). mp_charselect_page_down 0x407410 is exact** with the page
size in a local and `rows - page` written out at each use (`int page = mp_charselect_list_page[resolution_index][list];
if (pos < rows - page) { pos += page; pos = pos < rows - page ? pos : rows - page; }`): 8966 then loads the scroll
position first, keeps rows - page in ECX and the page in EAX for the add, as the binary. A `last` local (the old
form), a rows local without a page local, and the if / ternary / reversed min forms all give the other register
order. Same behaviour. Candidate for the other page-down functions that differ by register order only.

**D15701 (game/ai.cpp; m-1020d worker 3). ai_state::call_for_help 0x472200 is exact** with the range test split:
`if (!is_mode && target == -1) { float d = distance(&owner->pos, &c->pos); if (d < owner->info->detection_range)
add_goal9(...); }`. With the distance call inside the && chain, 8966 reads owner once before the call and keeps it
in a fourth callee-saved register for the goal arguments; the local makes it read owner again after the call (the
binary's `mov ecx, [edi + 0x4c]`, three saved registers). Same behaviour (same evaluation order).

**D15702 (game/level_scripts_common.cpp; m-1020d worker 3). level_check_triggers 0x4caa00 is exact** with the
same device for the radius trigger (`float d = distance_squared(...); if (d <= radius * radius)`: the radius is
loaded once after the call, `fld st(0); fmul`, as the binary; frame 0x3c) and the exit stores as name, script,
index (`level_exit_trigger`, `level_pending_exit_name`, `level_exit_trigger_index`; 3 of 9 orders match, this is
the one keeping block_flags first). Same behaviour. **Device to try elsewhere: a call result compared with a member
read gives the binary's reload when the result goes into a local first.**

**D15703 (notes only; m-1020d worker 3). Closer, kept; and tried without a match.**
Kept (same behaviour): mp_handle_actv_gm 0x470860, `e->ping = e->ping > 0 ? e->ping : 0;` instead of the
`&= (x <= 0) - 1` form gives the binary's store / and / store of the ping (536 of 537 bytes; the list unlink keeps
`&e->prev` in EAX in the binary, `lea eax, [ebp + 4]`; four statement orders tried); mp_charselect_handle_click
0x406bf0, `info_open == (((unsigned)flags >> 3) & 1)` gives the binary's `shr` and `cmp dl, cl` (the field stays
int; frame 8 against 0x14 and the loop registers left).
Tried: melee_damage (buf block-scoped in the MUG case: frame 0x294 -> 0x238 against 0x214; the binary puts buf
beside the first prepared-cast temporary, the 36-byte request slot apart; registers differ from the start;
reverted), ai_choose_special (the binary places the push test after the return; separate ifs, ||, switch, else-if
chains of the store), mp_reset_net_stats (the binary's second zero register: chained assignment, 7 placements of
the scalar stores, inline helpers for either half), cast_effect_track_add (loop forms, one-expression time),
party_set_level (the skill copy's SIB order [counter + info]: 8 index / pointer forms), ai_state::load (no `o`
local fixes the copy loop's rotation but hoists the owner load; type local, switch, block-scoped hash, a helper),
level_trigger_point_left_of_line (point array, a seg copy, a dead prev point, a result local), fire_weapon_missile
(`#pragma inline_depth(1..3)` changes nothing; the binary's orient assignment calls the vector3 copy constructor
for each row's dead result), get_protection (inverted armour test, e6 in p6), pick_attack_slot (declaration and
temporary orders), evil_joseph_pick_helper_summon (the midpoint's operand order: an explicit vector3 with y / z
swapped swaps y only; 8966 keeps z boss-first in every form).

**D15800 (game/new_itemtrade.cpp; m-1020d worker 4). itemtrade_handle_click 0x419710 is a byte match.** Five
changes, all same behaviour: (1) the button loop takes block-scoped pointers `ui_button_def *d =
&itemtrade_button_defs[i]; ui_button_state *b = &itemtrade_buttons[i];`, d declared first (the binary walks b in
EDI anchored at +0x10 and d in ESI, and the found block keeps using both pointers; with index forms 8966 rebuilds
the addresses from i; with b first ESI / EDI swap); (2) the flag-8 test is `(bool)((d->flags & 8) >> 3)`, which 8966
compiles to the binary's `shr eax, 3` (`(flags >> 3) & 1` on the int gives `sar`); (3) the gold limit is a clamped
local (`int limit = party_gold; if (limit >= ITEMTRADE_MAX_GOLD) limit = ITEMTRADE_MAX_GOLD;`; min() loads
party_gold before the amount arithmetic); (4) each point_in_rect rectangle is read into locals first (x, y, w at
function scope, reassigned for the party list; rx, ry, rw block-scoped in the row loops): the binary loads the
fields left to right before the pushes, as new_saveload does; (5) Esc is a one-case switch (`dec eax / je`, the case
body laid out last). Tip: the same block-pointer and rectangle-local shapes may fit the other screens' handle_click
loops (buysell, inventory, charselect).

**D15801 (vsdk/unnamed_quaternion_0x5225b0.cpp; notes). cquat::multiply 0x522a40:** the cross product in a named
local (`vector3 c = v1.cross(v2); v = v2 * w1 + v1 * w2 + c;`, as quaternion::multiply) gives the binary's middle
part (push order of the operator calls; 88 -> 31 differing lines). Still differs: the order of the component
conversions (`movsx` / temporary store / `fild`), the same in cquat_squad; int temporaries, a pointer to q, this->
and casts all compile the same. quaternion::to_matrix_b: 25 statement and factor orders of xy / wz and yz / wx all
give the same code (8966 orders the products itself; the binary loads w first in w * z and w * x).

**D15802 (game/unnamed_luminar_0x4c8d10.cpp; closer only). luminar_frame_hook 0x4c9000 and VC6's inline
budget.** The binary expands the first vector members and calls the COMDATs for later ones (vector3 copy 0x410cd0
from operator='s returned copy, vector3(x, y, z) 0x43b580). Which ones are expanded follows from the source's
operator use, and can be reproduced: with `p -= t->orient.uvec * luminar_pickup_tag_pos.y`, `vector3 d; d = p -
t->pos;`, `luminar_throw_velocity = d / reported_frametime;` (the divisor is operator/'s parameter copy: no ft
local), `ray.dir = luminar_throw_velocity * reported_frametime;`, `vector3 step = ray.dir * hit.t; h->pos = step +
h->pos;`, ray / hit declared where the ray is set up, the landing inside `if (landed) { ... }` (frame 0xac: its
fwd / up share slots with the grab block) and `vector3 fwd; fwd = h->orient.fvec;` with up's components stored, the
out-of-line calls are exactly the binary's (copy, copy, raycast, xyz, xyz, copy, ..., copy) and 268 -> 23 differing
lines remain: ray.start's three loads go through the EDI copy of &h->pos (the binary reads them off ESI and forms
EDI after). Observations on the budget (also D15803): a decision depends on the whole function, not only on the
statements before it (an extra declaration later changes an earlier expansion); a larger candidate is refused
while a later smaller one is still expanded; splitting one statement in two lets an operator inside a call argument
expand. Same behaviour throughout.

**D15803 (game/levelrender.cpp; closer only). levelrender_wave_lobj 0x48de60** with the same budget method:
`vector3 p = l->mesh->vertices[i];` block-scoped (frame 0xb8 as the binary), `float n = perlin_noise3(Space_mult * p
+ vector3(0.0f, 0.0f, Time_mult * t)); out[i].y += n * rec->weights[i] * rec->amplitude;` (the binary calls the
(x, y, z) constructor before s * v and expands operator+), the face's corners in `int a, b, c` read once, and n0's
second edge named (`vector3 e2 = W[c] - W[a]; n0 = (W[b] - W[a]).cross(e2);`): calls (rotate, xyz, s * v, perlin,
copy, xyz, xyz, xyz, copy, normalize x3, += x3, normalize) and frame are the binary's; 242 -> 51 differing lines,
scheduling only (the loop-2 count compare reads num_faces into ECX, the binary compares memory). Same behaviour.
The named edge is a device (it only moves the budget); a plain source with that effect was not found.

**D15804 (notes only; m-1020d worker 4). Tried without result:** xp_level_up_gains (ternary clamp, indexed forms,
if / else; EDI load order stays), character_can_talk_to (dz / dx locals, sqrtf, factor order: the binary keeps d.z
on the FPU stack with fld st(0)), ai_group_think_style2 (local declaration orders, i / e scopes: g stays in EDI where
the binary keeps it in memory), pick_test_character (store order x / z, test operand order, an h2 local),
world_pick_under_mouse (up as a constructor, len_sq, `*out = *out - ...`, a velocity local: 8966 always expands
plane::set_normal_point, which the binary calls at both sites; possibly its definition was not visible there).

**D15900 (game/new_pxochat.cpp; m-1020d worker 5). pxochat_drag_scrollbar 0x4244f0 is exact** in
mp_charselect_drag_scrollbar's form: pxochat_active_list read from the global at every use (no list local), the
sum written `thumb + (mouse_y - track_y - start_y)` (with the mouse first 8966 loads the globals in the other
order: a `target` mismatch, 13 other orders tried), and the row count read into `count` only after the clamp to 0
(8966 still CSEs it with the first read, giving the binary's EDI). Same behaviour.

**D15901 (game/level_script_worldmap.cpp; m-1020d worker 5). ls_worldmap1_pick_encounter_stage15 0x4fce40 is
exact** with the grassland branch's roll as a block-scoped `float r`: the binary's frame is dist -16, r2 -12,
best -8, r1 -4; with r2 shared by the forest and grassland branches 8966 swaps dist and r2 (all 24 declaration
orders compile the same). Same behaviour.

**D15902 (vsdk/unnamed_ix_0x53ac20.cpp; m-1020d worker 5). ix_ray_parallelogram 0x53af40 is exact.** The plane's
two corners are passed as `&(a + b)` temporaries (a VC6 extension; under `#ifdef _MSC_VER`, the named locals
remain for mingw), one float t serves both parameters (the binary keeps both in the dead edge argument slot), the
edge point at s is a named local q (`r = *hit - q`), and the second parameter is written
`((*hit - edge->origin).dot(perp)) / (perp.dot(perp))`: DEVICE (the parentheses only change 8966's scheduling of
the dot product's store, as D15200's length_quick). Same behaviour.

**D15903 (game/unnamed_pyrul_0x4b87c0.cpp; m-1020d worker 5; closer only). ai_state::pyrul_choose_attack
0x4b8da0:** the stage 2 / 3 `float weights2[2]` declared in their blocks give the binary's 0x68 frame (the array
shares the attackers slot, frame +4). Still differs: the binary lays out the `timer_up || !target` pick before the
retarget pick (ours the reverse); `!timer_up && target` nesting, `== NULL`, a goto-shared tail and a merged call
do not move it.

**D15904 (vsdk/unnamed_ix_0x53ac20.cpp; m-1020d worker 5; closer only). ix_ray_quad 0x53b130:** the two retries
call `pl.set_from_points(tri[0], tri[1], tri[2])` (the binary reloads tri[1] / tri[2] from the array and forwards
the stored new element); 677/685 -> 685/685 bytes. Still differs: the binary loads w and pushes float_min /
float_max's constants before `v + u` (13 forms of the mean and clamp tried). Same behaviour.

**D15905 (notes only; m-1020d worker 5). Tried without a match.** vector3::random_unit_vector / random_direction /
_in_cone (21 more forms of the sqrt argument: temporaries, casts, parentheses, double), vector3_angle_and_axis
(the acos result's fstp is scheduled after two pushes in the binary: 7 forms), rfx_preload (ECX / EDX roles of
offset / buf_size: 11 declaration and sum forms), pxochat_page_down (index / count forms, a row pointer, extern
const), pathvis_bsp_contains (case order and a goto into
case 1's recursion: worse), effects_parse_arr_line (the inner loop's first `>= 32` exit goes to the end epilogue in
the binary, to the shared loop-exit one in ours: break / for / while (1) forms), pathvis_add_edge_node (the binary
keeps &e->prev / &e->next in registers; not tried further), machival_ai_select_action (the binary keeps target in
a stack slot, no EBP: not tried).

## m-1020d worker 6 (D16000-D16099): s3d, worldmap / automap, spells, gr, gr_opengl, sndstream, vfx_channel, conversation

**D16000 (game/s3d.cpp; m-1020d worker 6). s3d_read_color_record 0x470300 is exact** with the three clamps written
through a file-scope macro `#define S3D_CLAMP(x, hi) (float_min(float_max((x), 0.0f), (hi)))` (vector.h's inline
helpers, D14602's in-memory clamp). Both the parenthesised argument and the parenthesised result matter: without the
argument parentheses 8966 copies rec->b / rec->g into the argument slot with integer moves (the binary: `fld; fst`),
without the outer ones it stores the 0.998 bound as an integer constant in a duplicated epilogue (the binary: `fstp
st(0); fld 0.998` into the shared store). The first clamp keeps the binary's integer copy either way. **VC6 finding:
a parenthesised float subexpression is not a no-op for 8966** (it changes how the value is loaded and stored). Same
behaviour (identical except for a NaN input).

**D16001 (game/unnamed_worldmap_automap_0x42f1e0.cpp; m-1020d worker 6). worldmap_marker_clamp_to_edge 0x42f870 is
exact** with the factors of the last two lines parenthesised, `offset[resolution_index] * ((float)cos(loc->angle))`
(likewise sin): 8966 then loads resolution_index before the fcos, as the binary does (D12107's open point;
`((float)cos(..)) * offset` also matches, the operand order is free). Same behaviour.

**D16002 (vsdk/gr_opengl.cpp; m-1020d worker 6). gr_opengl_create_texture 0x54e840 is exact** with two changes
that only match together: the scales as `(float)w / ((float)tex_w)` (the known `fild / fdivp` form instead of
`fidiv`), and the square-texture case through a local, `int m = tex_w > tex_h ? tex_w : tex_h; tex_w = m; tex_h =
m;` (the binary's `mov esi, ebp` / `mov ebp, esi` order and the two frame stores after the join). Same behaviour.

**D16003 (notes; m-1020d worker 6; closer only, kept, same behaviour).** gr_bitmap_float 0x507100: the eight corner
sums grouped as `(hs - wc) + x`, `(-ws - hc) + y`, ... (61 -> 18 differing lines with w6's relocation-resolving
diff): without the parentheses 8966 reassociates `-ws - hc + y` into `y - (ws + hc)`; the binary keeps the source
order (fchs, fsub, fadd). Left: the scheduling of verts[2]'s constant stores below the gr_tmapper pushes and the
ws / hc products after verts[0].sx (a statement-order hill climb reaches 10 lines with an unnatural order, not kept).
s3d_read_mesh_ref 0x46ffe0: `ref->_unk7 = (ubyte)hi;` gives the binary's dword load and `and 0xff` (31 -> 24); left:
the corner copies (the binary forms &box->maxs in a second register before the first copy; pointer / index / struct
forms tried).

**D16004 (notes only; m-1020d worker 6). Tried without a match.** gr_opengl_read_backbuffer (row pointer and index
forms, two-counter loops; D14108 already reached 9 lines); spells_parse_table's EBX / EDI swap (class store chains,
group pointer, loop forms; the binary also always uses EDX for `&tf` in the effect chain where 8966 rotates EAX / ECX
/ EDX); ls_ikaemostopint_init's constant 1 in EBX (bool / int locals, store orders, chained stores: 8966 keeps it in
one block only, as in the matching ls_ikaemosbottomint2_init); wav_open's imul operands (parenthesised forms);
snd_stream_fill_pcm (DWORD / UINT local types: the binary also keeps n2 in written's argument slot); summon_create
(no flag set stops 8966 cross-jumping the repeated get_int tails; the binary's 0xc0 / 0xcd pair is the only merged
one); spell_roll_amount (11 more loop forms: `i <= count - 1`, `--i >= 0` give `dec / lea` but test after the dec);
worldmap_frame (parenthesised and explicit sqrt forms); gr_line, gr_tmapper_multipass, gl_convert_1555_to_rgba8888,
vfx_channel::eval, fx_rec4c::update, restore_state, the fill_* functions (a one-at-a-time parenthesis sweep over
every call, cast and operand: no change); gr_init's stores (`window_mode` first is closer, 136 -> 120; a hill
climb reaches 92; the binary's `lea edx, [eax - 1]` for clip_right stays `dec`); s3d_read_door (a store-order climb
and seven return forms: only `mov ecx, edi` stays one block early).

**D16100 (game/unnamed_script_editor_0x4de0b0.cpp; m-1020d worker 7). script_action_format 0x4e4910 is exact**
with three source changes: the type read once into a local (`int type = a->type;`, the binary keeps it in EDX for
the name lookup and the switch), the case bodies in the binary's layout order (walk, wait at, do state, moves,
.mvf / .vfx, say, the "%d" group, synch teleport, wander, wander for, wait go, pass go, follow, replace weapon /
armor; read off the jump table 0x4e4b50), and an explicit `default: break;` at the end of the switch. Without the
default 8966 cross-jumps the sprintf tails differently (synch teleport into follow, say not into wait at) although
the bodies are the same; with it the tail merges are the binary's. Same behaviour. A device to try on other
switches whose tail merges differ (it changed nothing in script_create, add_navpoint, handle_input,
door_sound_inputbox_cb, fill_action_lists or ui_button::process, whose switches end the function).

**D16101 (game/unnamed_script_editor_0x4de0b0.cpp; m-1020d worker 7). script_editor_add_navpoint 0x4e5b10 is
exact** with the cutscene case's point written `pos = ray.dir * hit.t + ray.start;`: the inline vector3
operator+ then loads each product component first and adds the ray start, as the binary does (`fld tmp.y /
fadd start.y`). IEEE addition is commutative: same behaviour. The level case keeps `ray.start + ray.dir * hit.t`.

**D16102 (vsdk/unnamed_gr_line_0x53e520.cpp; m-1020d worker 7). gr_draw_sphere 0x53f720 is exact** with the two
scaled radii as float locals (`rs = sa * radius; rc = ca * radius;` and in the loop `rs = (float)sin(angle) *
radius; ...`, then `p = *pos + m.rvec * rs + m.uvec * rc;`): the binary stores both products to the frame before
the vector3 calls (frame 0x214, ours was 0x20c), and `angle = 0.0f;` before `step = ...` (the order decides which
of the two slots angle gets). Same results: the binary rounds the products to float too.

**D16103 (game/unnamed_script_editor_0x4de0b0.cpp; m-1020d worker 7). script_editor_ray_hit_object 0x4e2f30 is
exact** with the distance assigned to the radius parameter (`radius = distance(pos, &hit);`) instead of a `float d`
local: the binary stores it into radius's argument slot (`fst [esp+0x58]`) and copies it to
script_editor_pick_distance with integer moves. The change also gives the binary's partly expanded hover
orientation copy (row 0 inline with the out-of-line copy constructor, rows 1-2 through vector3::operator=): VC6's
inline budget depends on the rest of the function. Same behaviour (radius is not used after the sphere test).

**D16104 (game/unnamed_script_editor_0x4de0b0.cpp; m-1020d worker 7). script_editor_focus_selection 0x4e75c0 is
exact.** The binary expands matrix::operator='s first two row copies with out-of-line copy constructor calls for
their results and calls vector3::operator= for the third (D10808's "inline budget"). The budget follows the
number of inline expansions in the function: three source changes together give the binary's split: the
selection object read at each use (no `Character *c` local), the size class row copied as a vector3
(`script_editor_sel.size = *(vector3 *)script_editor_char_sizes[...]`, as script_editor_create_ui already does;
the binary copies it with three integer moves, so the table is probably `vector3[4]` in the original), and the
navpoint box size through the inline `script_editor_sel.size.set(0.8f, 0.4f, 0.8f)` (vector3(0.8f, 0.4f, 0.8f)
assigned gives the same code). Method: count the binary's out-of-line copy constructor / operator= calls and add
or remove inline member uses elsewhere in the function until ours expands the same ones. Same behaviour.

**D16105 (notes only; m-1020d worker 7). Tried without a match.** snow_start 0x494ab0: the binary's outer test is
signed (`jl`), so the outer loop is an int loop over i with the cells indexed `snow_cells[i][j]`; with a walking
`row` pointer (`rec = row; row += SNOW_GRID;`) the `jl` and the size are right, but the row pointer is loaded before
the cells pointer and `add ebp, 0xb4` comes before `mov ebx, 15` (12 forms; not kept). select_prev 0x4e7460: a
`Character *next` local (`next = obj->live_prev` before the loop and at the end of the body, `while (obj != next)`)
gives the binary's size and loop rotation; only next's register (ECX, binary EAX) differs (not kept).
action_type_allowed (the end-of-switch `return false` merges with the mount case's instead of the do-state loop
exit; break / return / if-block forms change nothing), apply_action_params (the binary duplicates the line update
tail per case; default forms change nothing), ls_ionaext_frame (the `sete`: locals, `== false`, ternary, inline
helpers, volatile), mp_handle_ai_disable (from in ECX: case order, default, cast), mp_send_new_lep (header / info
order), light_create_spot (clamp order, local k), gr_bitmap_3d_oriented_quad (8 store orders of v[3]),
http_async_gethostbyname (all 120 orders of the five record stores and two global orders), door_sound_inputbox_cb
(case order), item_obj_spawn (the binary's /Od copy of orient is the D16104 inline-budget split: pos inline, rows 0-1
with copy constructor calls, row 2 through vector3::operator=; not tried further).

**D16200 (vsdk/unnamed_ui_slider_0x55e040.cpp; m-1020d worker 8). ui_draw_bevel 0x55e040 is exact** with the
corners computed into the parameters (`x2 = x1 + x2 - 1 - inset; y2 = ...; x1 += inset; y1 += inset;`, the
parameters renamed x1, y1, x2, y2). The binary keeps the four corners in the x, y, w, h argument slots; with
separate locals 8966 builds a frame of its own (`push ecx`). Same behaviour. Technique: when the binary stores a
value derived from a parameter into that parameter's slot, write it as an assignment to the parameter.

**D16201 (vsdk/unnamed_poly_split_0x55f110.cpp; m-1020d worker 8). gr_poly_get_section 0x55fb00 is exact**: the
clamped bitmap size is written back into bw / bh (`bw = imax(bw, max_bitmap_w)`; the binary's imax result goes
to bw's slot, and with separate sw / sh 8966 shares `(float)sw` across statements), and the mean-uv loop reads
through `gr_multipass_vertex *v = verts[i]` (indexed `[eax + ecx + 4]` addressing). **LOGIC FIX**: the scales are
`(float)bw / max_bitmap_w` and `(float)bh / max_bitmap_h` (0x55fc1e and 0x55fc3c are `fdiv st, st(1)`, d8 f1,
with st = the bitmap size); the decomp had the inverse. The callers (gr_d3d_tmapper_multipass, the Glide
multitexture path) compute `(u - u_off) * u_scale`, which needs bitmap / card.

**D16202 (vsdk/unnamed_vfile_rw_0x525720.cpp; m-1020d worker 8; notes only). File::read 0x525fe0** still differs
(84 -> 54 differing lines): `int total = len;` first, `int avail = s->buf_len - s->buf_pos;`, the remainder as
`int rem = len - n` (EDI) and the refill size assigned to len (the binary's len slot holds it). Left: the binary
computes `buf_start = n + pos` as `mov ecx, [pos]; mov eax, ebp; add eax, ecx` (8966: `mov eax, [pos]; add eax,
ebp`); eight forms tried (operand orders, a pos / start local, statement order, max forms).

**D16203 (game/unnamed_goals_doors_0x475180.cpp; m-1020d worker 8; notes only). matrix3_unpack_orientation
0x475430**: the two pairs as `vector2 r = *(const vector2 *)&in->r0; vector2 u = ...` give the binary's integer
copies (111 -> 55 differing lines). Left: the frame (0x14 against 0xc: the binary keeps rz / uz in the dead in /
out slots) and the sign multiplies (`fld s; fld x; fmul st, st(1); fstp; fstp st(0)`; ternary-first, a sign
local, if / else, `*=`, four inline helpers tried). matrix3_pack_orientation: the binary also uses the two
dead argument slots (frame 8 against 16); declaring the second copy after the first stores gives frame 8 but
drops the second copy's stack round trip; block scopes, register parameters, local copies tried.

**D16204 (vsdk/gr_direct3d.cpp; m-1020d worker 8; notes only). gr_d3d_init_device 0x5395c0**: the clipper (windowed
block), fog_start / fog_end (the vertex-fog else block) and total_mem / free_mem (a block around the video
memory check) block-scoped: 8966 then shares their slots as the binary does (clipper / fog_start / free_mem at
one slot, fog_end / total_mem at another), the frame is 0x2067c and the first difference moves from +0 to the
texture-caps tests, where the binary computes `((caps & 0xff) >> 1) & 1` and `((caps & 0xff) >> 5) & 1` from
two copies. Tried for that: ? true : false, != 0, bool / BYTE / unsigned char casts, a BYTE local (goes to the
frame), a DWORD local cast per test (one CSE'd mask), bool helper functions (not inlined, /Ob0).

**D16205 (m-1020d worker 8; notes only). Tried without a match:** ui_gadget::send_event (ESI / EDI swap: plain
call, parent local, else chain, handled outside, this->id, ternary value, != forms), ui_listbox::wrap_text
(cur_indent in EBP where the binary keeps split: split / lw / next scopes, compare form), scroll_page_up
(the last block's vscroll register: y local, operand orders, block local, else-if, inverted test),
create_scrollbar (lb everywhere, no lb, this->, view first, a self pointer), door_find_floor_point (mid
block-scoped in the layer loop, point after found: no frame change), gr_poly_split_edge_vertex (not tried in depth:
the binary keeps g / b in the npasses / passes slots).

**D16300 (game/unnamed_ai_move_0x4b1410.cpp; m-1020d worker 9). ai_state::path_closest_point 0x4b2670 matches.**
The segment passed to seg2d_closest_point is a `pf_edge` (16 bytes, a seg2d plus flags / door / search id), not a
bare seg2d (8): the binary's 0x60 frame has the segment at the top of the locals with 8 unused bytes above its two
pointers, which is the rest of a pf_edge (ai_find_nearest_wall_hit uses pf_edge locals for its segments too). With
it: `found` / `n` initialised before the profile label (their register stores precede the timer_get push), the
look-ahead point p and the back point q in two sibling blocks (they share one slot), best and closest zeroed by
`best.x = best.y = 0.0f; closest.x = closest.y = 0.0f;` (the binary stores best.y, best.x, closest.y, closest.x),
and `owner->pos + owner->centre_offset` (fld pos, fadd centre in the binary). Same behaviour. General lesson: when
the binary's frame is larger than ours by the size of a struct tail, look for a local declared with too small a
type.

**D16301 (vsdk/unnamed_localize_0x503f60.cpp; m-1020d worker 9). Index loops, not walking pointers:
localize_unescape 0x503fe0 and localize_remap_charset 0x5051a0 match.** The binary tests `cmp byte ptr [ebp], 0`
before setting up its walking register and reloads the character at the loop top: that is `for (i = 0; text[i];
i++)` with text[i] reads (8966 strength-reduces i into a pointer after the first test, and the empty-string path
gets its own copy of the epilogue, so ESI is pushed only around the loop). The old `for (p = text; *p; p++)` gave
a CSE'd first load and an entry push. In localize_unescape the hex digits are `hex[2] = text[i++]; hex[3] =
text[i];` after `i++`. Same behaviour.

**D16302 (vsdk/unnamed_localize_0x503f60.cpp; m-1020d worker 9). localize_load_level_table 0x504870 matches.**
The same index form for the language-name lowering (`for (i = 0; lang[i]; i++) lang[i] = tolower(lang[i])`) and
both hash passes (`i = 0; while (...) hashes[i++] = ...` and `localize_table_add(text, hashes[i++], table)`): the
binary sets the hash pointer after the first Accept test. In the script branch `braces = false` comes after the
strcat (the topic branch keeps `braces = true` after the filename strcpy); `table` may stay where it was. Same
behaviour; the p / h locals are gone.

**D16303 (game/unnamed_ai_move_0x4b1410.cpp; m-1020d worker 9; closer only). ai_state::travel_steer_update
0x4b3c80 (nd 187 -> 12 normalised instructions):** `vector3 *dest = &travel_dest` before turn_towards (the
binary keeps the pointer in a frame slot and reloads it for the distance), one function-scope `vector3 d` for the
party member offset (`d = c->pos - owner->pos - owner->centre_offset;` assigned, not initialised) and for the final
distance (`d = vector3(dest->x - o->pos.x, 0.0f, dest->z - o->pos.z); dist = sqrt(d.z * d.z + d.x * d.x)`: the
binary keeps the dead x / 0 / z stores because d's address escapes to normalize_safe_length), and `d =
owner->orient.fvec + owner->orient.rvec` for the left step. Still differs: the facing dot product, which 8966
emits y, z, x for every term order, operand order, d.dot / fvec.dot form (the binary: z, y, x), and the y / z
operand order of the side vector add. Same behaviour (float adds commute).

**D16304 (notes only; m-1020d worker 9). Tried, not reproduced.** ai_state::turn_towards 0x4b2af0: the frame
differs in two swaps (origin above t / angle, right below fwd in the binary); declaration order, late
declarations, separate declarations, function-scope or one-block origins and `&(const vector3 &)vector3(0,0,0)`
temporaries do not move them (8966 /O2 ignores declaration order here). ai_find_nearest_wall_hit 0x4b15a0: the
binary keeps `from` in EBP and reloads `dir` per edge, which pushes all four registers at the entry; early returns,
a normal vector local, term order and an index loop over the obstacles (closer, not kept: 93 -> 89) do not change
the allocation. glide_record::alloc 0x549b50: the final next / addr and texture / section store order (three more
orders, a return local, a duplicated tail). ca_character::report_memory 0x541020: 14 more groupings, three-statement
and local-sum forms all compile to the same canonical sum (the binary adds pos * 64 to skel first).
localize_set_string, localize_load_names, vfx_instance::init_nodes: looked at, earlier notes' findings stand.

**D16400 (game/new_levelselect.cpp; m-1020d worker 10). mp_levelselect_drag_scrollbar 0x41ce50 byte match.** D13100's
shape (mp_charselect_drag_scrollbar): the list index read from mp_levelselect_drag_list at every use (no local), the
row count loaded again after the clamp to 0 (`int count = ...` placed there) and the drag offset written
`thumb + (mouse_y - track_y - start_y)`. With a `list` local 8966 holds it in ESI (binary EBX). Same behaviour.

**D16401 (vsdk/math/ix.h; m-1020d worker 10; header). plane::transform returns the plane by value; resolves D4502's
constructor point.** `plane transform(const vector3 *pos, const matrix *orient)` with `return p;`: the caller's
`plane p = gr_frustum_planes[i].pl.transform(pos, orient);` is the hidden result, so it gets no constructor call,
and gr_cull_bounding_box_transformed 0x50ee70 is a byte match. Evidence: 0x50f880 ends `ret 0xc` (hidden pointer +
pos + orient), returns the first argument, and its own local is built by the folded 0x5081c0 (the default
constructor of a class whose vector3 member is constructed), and its last call (this = result, arg = &local) is
the copy of `return p` into the result, i.e. plane's implicit copy constructor. With vector3's user copy
constructor that copy calls 0x410cd0 under /Ob0, so the 0x4c0a20 the binary calls (four dword moves) is the
copy constructor emitted by an optimised object (the projector unit expands vector3's copy), linker-folded with
the projector's plane::operator= of identical bytes. So D4502's "two plane assignments" need not be two types:
0x50f760 is the operator=, 0x4c0a20 the copy constructor / folded assignment. The only caller of transform is
0x50ee70 (checked across the tree). The 0x50f880 COMDAT itself is not compared; ours differs from it only in one
fstp of p.d scheduled two pushes later. mingw compiles all units (decals keeps its 9 errors).

**D16402 (game/conversationctl.cpp; m-1020d worker 10). conversation_controller::camera_capture 0x4d8e10 byte
match.** `game.set(game_camera.get_pos(), matrix43(game_camera.get_orient()).transform(vector3(0.0f, 0.0f, 1.0f)));`
The forward vector is a temporary bound to transform's reference parameter: 8966 then calls its constructor
(0x43b580) and expands transform with real multiplies, as the binary; with a named `vector3 fwd(0, 0, 1)` local
the constructor is expanded and the products folded to constants (29 instructions short). matrix43::transform's
own expression gives the binary's operand order; the file's conversation_transform helper is gone (its 96 term /
factor orders all compiled the same). Same behaviour.

**D16403 (game/multicontainer.cpp; m-1020d worker 10; closer only). mp_handle_item_drop 0x489120:** the matrix is
`matrix(vector3(1, 0, 0), vector3(0, 1, 0), vector3(0, 0, 1))` with temporaries, built right to left as the binary
stores them (fvec's three stores first; named rvec / uvec / fvec locals store rvec first). Still differs: the binary
copies mp_dequantize_position's result from a temporary into pos (`vector3 pos; pos = ...`, `vector3 pos(...)`,
`= vector3(...)`, a named temporary: none gives it; two disturb the matrix copy), gold in EDI and the item pointer
in memory.

**D16404 (m-1020d worker 10). Tried, not kept.** phoenix_choose_attack 0x498a20: `if (owner->combat.is_busy() ==
true) { if (phoenix_cur_attack) return 5; } else if (phoenix_cur_attack) return 5;` gives the binary's dead
`cmp al, 1` (the merged branches leave the compare); the unused `bool busy`, an empty if, an expression statement
and `if (...) ;` do not. Not kept (the global load before `sub esp`, `this` in EBP unspilled and the stage-3 timer
values in a frame slot still differ). character_use_skill's pick-lock fcompp: double / float casts, a local, the
gt / lt swapped if-else and `!(lock > s)` (none). gr_setup_3d's threaded Glide test: ternary forms and the branch
order (none). titus_ai_select_action: `!target || timer_up`, the inverted nested form (none). last_visible_line (8
loop / local forms), mp_player_file_read_summary's flag remap (7 forms), gr_get_frustum_planes (10 forms): register
choice only.

## m-1020d worker 11 (D16500-D16506)

**D16500 (game/gameplay.cpp; device). game_render_frame 0x438dc0:** `cursor_visible = (unsigned char)~*(volatile
int *)&game_flags >> 5 & 1;` matches. Sum.exe reads game_flags as a dword and then works on AL (`not al / shr al,
5 / and al, 1`); 8966 narrows every plain byte form to `mov al, [game_flags]` (D13702) and the plain
`!(game_flags & GF_CUTSCENE)` shifts first. The volatile dword read stops the narrowing. Not plausible original
source (game_flags is probably a struct of bit-fields, D6703); same value. The same device may fit
game_is_paused 0x432da0 (`shr al, 2`, not in this worker's files; untested).

**D16501 (game/script_internal.cpp). script_action_combat_mode 0x4dcbc0:** `bool teleports_only = true;`
declared (and initialised) after the species test instead of at the top. 8966 then keeps skip in AL
(`xor al, al ... mov al, 1`), teleports_only in BL, c in EBP and `on` in a's dead argument slot, without the
extra `push ecx` frame slot. The 24 declaration orders tried earlier all initialised at the top; the point of
the initialisation is what counts.

**D16502 (vsdk/character_instance.cpp). character_instance::sum_root_velocity 0x51abd0:** the NULL test reads
`character->skeletons[i]` directly and `skeleton *s = character->skeletons[i];` starts after it. 8966 then
strength-reduces the skeleton address into an offset induction variable (`mov edi, 0x54`, `[edi + eax]`,
`[edx + edi - 0x54]` for times[i]) and keeps this in ESI, i in EBX, &root_velocity in EBP as Sum.exe does.

**D16503 (game/unnamed_interface_0x411270.cpp; closer only). interface_init_global 0x4116c0:** three changes,
71 -> 21 differing lines, size now equal: `float fov = camera_fov;` for the gr_setup_3d argument (fld / fstp, as
D10307); the screen width as `screen_w = 640.0f; if (res == 1) ... else if (res == 2) ...` (Sum.exe's `cmp
eax, 1 / jne / fstp st(0) / fld` chain keeps screen_w on the x87 stack); `base_w = 640.0f;` before the first
switch puts base_w in memory (`fdiv dword ptr [esp + 4]`). Left: Sum.exe stores 640 inside the out-of-line
0 / 1 / default block; with the stores there (any case order, merged labels) 8966 keeps base_w on the FPU.

**D16504 (game/unnamed_interface_0x411270.cpp; closer only). ui_choice_popup_show 0x4136a0:** `lim =
max(ui_choice_popup_w, w); ui_choice_popup_w = lim;` gives the loop body (popup_w loaded first, both stores
sunk to the loop end). Left: the counter is ESI and the choice pointer EDI here, EDI / ESI in Sum.exe (i is
zeroed after the title strlen there); declaration orders, a second loop variable and a pointer loop tried.
ui_choice_popup_draw: a `char **p` walked in the for gives the binary's EBX pointer / EBP x, but the pointer
set-up is hoisted above the count test and popup_x is added after the hover load (not kept).

**D16505 (vsdk/bmpman.cpp; closer only). bm_load_internal 0x502720:** `reduced = bm_reduce_textures && num_levels
> 1 && num_frames == 1; if (reduced) ...` (83 -> 65 lines). Sum.exe keeps reduced in BL and the constant 1 in
EAX (`mov bl, al`), and branches to the M2V total-size case.

**D16506 (notes only, m-1020d worker 11). Tried without a match:** mp_handle_it_okay (the binary re-tests
state 3 on the state-2 path and threads only the state-3 path: ternary / bool / int / `!= 0` value forms
thread neither path, else-if / switch / early-return forms thread both; 20 forms); summoner_cfg_load (the
tail's register rotation EAX / ECX / EDX is one step behind ours: clamp forms, `!f.open`, pxo flag forms do not
move it); fast_inv_sqrt / fast_sqrt (#pragma optimize w / a / y / p / s / t / g change nothing or more);
bm_lock_pcx (`[8 * esi]` with esi = 9 * index: entry pointer, bpp local, pragmas); bm_set_reduced (a shared
level local as char / ubyte / int; char gives the early push but sign-extends); script_action_replace_weapon
(late and reordered declarations, block-scoped s, `c ==` order); script_action_walk_to (nine declaration
placements: identical code); script_action_replace_armor (Sum.exe's `sete al / cmp al, bl` on the slot-8 test:
bool locals, `== true`, inline helpers all fold to a jump); ui_message_box_show (volatile byte reads give the
reread and `mov al, 1`, but the flags / callback registers stay swapped); bm_lock_vbm (size expression orders,
walking and indexed frame loops: identical code); ui_3d_begin (not retried beyond a look).

**D16600 (game/unnamed_fx_0x489c40.cpp; m-1020d worker 12). fx_create_at_camera 0x48c140 matches, and the
old source turned the copy the wrong way.** The binary (0x48c14b-0x48c1ff) copies the camera orientation, saves
the old up and forward, negates right and sets up = -(old forward), forward = -(old up): the same turn as
fx_frame's kind 3 (D9815). Written that way (`vector3 u(m.uvec); vector3 f(m.fvec); ... m.uvec = -f; m.fvec =
-u;`) it is a strict byte match. The old source negated all three rows. The copy is never read (the instance gets
camera_view_orient), so this changes nothing in game.

**D16601 (game/unnamed_fx_0x489c40.cpp; m-1020d worker 12). fx_create_instance 0x48bdc0 matches.** Three
changes, all needed together: the default orientation as a conditional expression,
`fx->orient = orient ? *orient : matrix(vector3(1,0,0), vector3(0,1,0), vector3(0,0,1));` (the binary copies from
the constructor's return in EAX and shares the 9-dword copy with the non-NULL path; a block-scoped `matrix
ident(...)` gives the right 0x6c frame but an extra `lea eax, ident`); the random yaw as `float a = frand() *
2pi; matrix rot(0, 0, a); fx->orient = rot * fx->orient;` (frand before the operator* pushes, the float stored
in its frame slot); and the reset_flags call as if / else of two calls (`push 0 / je / push 0 / jmp / push 1`,
cross-jumped). With the old `ident` local at function scope the matrix copies called vector3's copy constructor
and operator= out of line (frame 0x9c).

**D16602 (notes only; m-1020d worker 12). Closer, kept, not exact:** ai_state::serpent_choose_attack 0x496330:
the three mode tests are `owner->ai.is_mode(...)` (the binary loads owner +0x4c and adds 0x324 for each call,
0x49636b-0x4963b9), not `this->is_mode`. The same object when the state belongs to its owner; the code now does
what the binary does. Still differs: idx in ESI (ours EBX), busy_slot spilled to the frame before the test, frame
0xa0 vs 0xb0.

**D16603 (notes only; m-1020d worker 12). Tried, still differs:** camera_blend_sequence::advance 0x494400: the
binary's angle slots go h0, h1, b0, b1, p0, p1 upward and it multiplies h0 * u first. Pairs of arrays
(`float p[2], b[2], h[2];`) give exactly that slot and evaluation order, but the frame is 0x30 against 0x40 (the
vector temporaries then overlap the arrays). Declaration orders, swapped blend terms and variable names do not
change the scalar layout (target before camera in each pair). fx_find_spawn_point 0x48d2b0: the end points as
`ends[i] = vector2(a, b)` give the binary's per-end temporary and a frame 0x88 against 0x8c (was 0x7c); c in
ESI (binary EBP); not applied. characterinfo_alloc_level_buffers 0x448800 (strict target only): the four-size sum
loads rosalind / flece / jekhar / joseph in the binary, ours jekhar / flece / joseph / rosalind; all 24 operand
orders, extern declarations placed earlier and the store order of the last branch compile the same order (the
order seems to come from the symbol, not the source). ds_is_3d_buffer (`shr al, 4; and al, 1` after a dword
load: 15 expression forms), TextFile::ParseInt (this / value in EDI / ESI: 21 forms; hoisting `value *= 16`
out of the hex branches swaps the registers but moves the shift), pickup_draw_item_list (x / i EBX / EBP: 8
declaration forms), serpent_illusion_start (scratch registers: 6 forms), living_entity_setup_head (the second
loop's scratch registers: 8 forms), read_track's SIB order (index types int / unsigned / short / ushort / uchar /
long).

**D16700 (game/unnamed_shadows_0x4a37d0.cpp; m-1020d worker 13). shadow_project_face 0x4a46a0 is exact** with
three changes together: the vertex indexed as `buf.v[i]` in every use (no walking `v` pointer; 8966 then keeps
the unbiased vertex address in EDI as the binary does, not &v->a in EBP), `vector3 d = clipped[i] -
s->local_pos;` as a loop-body local built by operator- (it fixes the place of `lea ecx, d`), and `vector3 world =
l->pos + clipped[i].unrotated(l->orient);` (pos first: the binary's x sum loads the rotated x and adds pos.x,
the other order commutes the x sum only). Same behaviour (float addition commutes exactly).

**D16701 (vsdk/unnamed_ca_mesh_0x50af80.cpp; m-1020d worker 13; closer only). ca_render_morph_vertices
0x50b150** (LCS distance 30 -> 17): `frame > m->keys[key - 1].time` gives the binary's `fld st(1); fcompp; test
ah, 0x41`; the last key read as `m->keys[m->num_keys - 1].v.x` three times (no `last` pointer); the anim choice
as a ternary; a `p` pointer for key - 1; and the buffer written through `float *mv = (float *)ca_morph_vertices`
as `mv[v * 3 + c]`. DEVICE: mv is not plausible original source; it is what reproduces the binary's `lea ecx, [v
+ 2v]` with `[4*ecx + base]` stores (a vector3 index gives `shl` by 2 and `[eax + base]`). Still differs: the
binary keeps ca_render_instance in no register (EAX at the top, EDX in the loop) and pushes EBX / ESI / EDI / EBP
late; ours holds it in EBP and pushes at once. Same behaviour.

**D16702 (game/unnamed_shadows_0x4a37d0.cpp; m-1020d worker 13; closer only). obstacles_debug_render 0x4a3890**
(LCS 150 -> 21, 1028 -> ~1340 bytes of 1351). The binary works the segment loop in vector2 operators: `vector2 m
= 0.5f * (cb->pos + ca->pos)`, `n = 0.5f * s->normal`, `t = m + n`, and the offset points `a` / `b` as `0.5f *
offset + pos`, then copies components into p0 / p1 / mid. The macro's points are assigned (`vector3 a =
pa->to_vector3(); p0 = a;`: the copy reads the named result's frame slot, not EAX), the segment-list loop is an
int index (`jl`, not the pointer compare's `jb`), num_segs is read once and the corner indices are locals reused
for avoid_record_axis_at. BEHAVIOUR (the binary's, now matched): mid.y is never written in the segment loop, so
the normal tick starts at a stale frame value; the old source wrote the debug height there (debug drawing only).
Still differs: the frame (0xa8 against 0x9c; block-scoping mid, m / n / t / a / b at function scope do not move
it) and the scheduling of the corner loads.

**D16703 (vsdk/unnamed_vfx_emitter_0x55ff00.cpp; m-1020d worker 13; closer only). fx_recb0::move_particles
0x560a00** (LCS 26 -> 4, frame now the binary's 0xa8): `first = (int)ceil(last_t);` as in spawn (D13604); with
`ceil(prev)` 8966 keeps (double)prev in an 8-byte frame slot and reuses it for the floor calls, while the binary
converts prev at each call. Left: spawn's own difference (prev stored before the fstp of the ceil argument; 12
forms tried: comma expressions, ceilf, a double local, statement orders, `ceil(prev = last_t)`) and one `mov
ecx, [eax]` placement in the second force block (block conditions and `total` placement tried).

**D16704 (game/unnamed_shadows_0x4a37d0.cpp; m-1020d worker 13; closer only). shadow_transform_to_lobj 0x4a4890**
(LCS 130 -> 62, frame now 0x5c): the door pose as `pos = door->closed_pos * g + door->open_pos * f;` (open /
closed branches `pos = door->open_pos;`), `s->local_pos = s->pos - pos;` (the binary's copy-constructor call after
it is operator='s returned copy). Still differs: which inline vector3 members 8966 expands. The binary calls
operator= (0x413190), the (x, y, z) constructor (0x43b580), the copy constructor (0x410cd0) and matrix3__copy out
of line at some sites and expands them at others, and neither `#pragma inline_depth(0..3)` nor padding with
inline calls reproduces the pattern (shadow_record_add 0x4a3ef0 has the same problem: its orient rows and
shadow_frame copy are out-of-line calls in the binary; 28 forms tried, not kept).

**D16705 (notes only; m-1020d worker 13). Tried without a match:** chat_input_handle_key (Esc / Enter / Backspace
case orders, inverted Enter test, default label: the cancel block stays last), lobj_raycast_draw_lists (ESI /
EDI: block-scoped i, declaration orders, store orders), pcx_read_bitmap (pos / count orders, `count` / `pos` as
the zero arguments), anim_track::calc_rot_tangents, fx_recb0::get_fade, init_particle (prior notes), the ca_mesh
renderers (looked at: the OpenGL one keeps 0 in EBP and a vector3 temporary the source lacks).

**D16800 (game/level_scripts.cpp; m-1020d worker 14). savegame::write_party_state 0x4cf500 matches:** the solo
bit as `(game_flags & GF_SOLO_MODE) != 0` and `selected` as one conditional store
(`ps.selected = (flags & CF_SUMMON) ? -1 : species;`). With the if / else store 8966 cannot merge the
bit fields; with one store it merges selected and `summon = -1` into the binary's `or esi, -8; shl esi, 7`,
and the solo bit becomes `shr 7 / and 0x20`. Same values (a 1-bit unsigned field).

**D16801 (game/unnamed_effectstream_0x440400.cpp). effect_stream_finish_entry 0x440930 matches** with
`case -1: return 0;` written last in the switch: its return 0 then shares the error tail after the switch
(the binary's table entry 0 points into it) and the .rfx case falls into the effect_stream_next call. The
case order is not sorted away by 8966 for block placement: all 24 orders of the four cases were compiled,
only this one matches.

**D16802 (game/level_scripts.cpp; closer only). conversation_execute_action 0x4d5990:** the two formats
of the sprintf calls with a call argument read into a `char *fmt` first (the binary loads them into EBX /
EDI before character_get_name; the form noted for party_member_join). Kind 2 then no longer uses EBX and
8966 saves EBX only inside kind 1 (shrink-wrapped, as the binary). Kind 11 written as
`if (party_gold >= a->value) true; else false;` gives the binary's shared call with kind 9's false call.
Still differs: kind 7's false call (the binary jumps back into kind 7's own call after `push 0`; 8966
merges it with kind 9's; 11 kind 7 / kind 9 forms tried).

**D16803 (game/new_dialogue.cpp; closer only). dialogue_frame 0x409000:** the second mode update as
`if (!built) { mode = 3; if (count != 0) return key; } mode = 1;` gives the binary's store of 3 that is
overwritten by 1. Still differs: the binary loads `key` into EDI before the tail's `mode != 0` test and
shares one exit with that path; ours duplicates the exit (11 forms: else-if chains, goto, early return).

**D16804 (game/unnamed_effectstream_0x440400.cpp). effect_stream_start 0x440560 matches** with one
`return 1;` after nested if / else (DONE, DONE, READING stores in the branches). With a `return 1` after
each store 8966 CSEs the READING constant 1 with the return value (`mov eax, 1; mov [state], eax`); with a
shared return the store and the return are in different blocks and 8966 then copies `mov eax, 1; ret`
into each path, the binary's code. Worth trying wherever a store of 1 is followed by `return 1`.

**D16805 (game/level_scripts.cpp). level_item_has 0x4d0dd0, level_item_remove 0x4d0d60 and
read_level_items 0x4d33f0 match** with the search written as level_item_add's: `ubyte *data =
level_item_hashes.data; count = *(int *)data; off = 4;` and `h = *(unsigned int *)(data + off); off += 4;`
in an index loop. 8966 then creates the walking pointer after the count test and keeps the re-test of
`i != count` after the loop (D10404 (2)). read_level_items also needs the level item through a record
pointer (`level_item_rec *r = &level_items[it->level_item]; name_hash_nocase(r->name)`) for its EAX / ECX.

**D16806 (game/level_scripts.cpp; closer only). savegame::get_speaker_bit 0x4d05f0:** the same byte-offset
search, the count read through CONVERSATION_SAVE_INFO in the loop condition and in the re-test (with a local
`n` 8966 threads the `i >= n` test away; with `==` it keeps it but as je), and `int bit = i + kind;
((1 << (bit & 7)) & (data + (bit >> 3))[bits_off]) != 0`. Every instruction but the data / bits_off
register pair (EDI / EBP in the binary, EBP / EBX in ours) and the pops scheduled with them matches; six
declaration orders and three bits_off placements do not change it.

**D16807 (game/level_scripts.cpp). savegame::level_item_add 0x4d0d00 matches** with every access through
`level_item_hashes.data` (no `data` local): the hash load and store become `[edx + ecx]` (data + off),
the binary's base / index order; with the local 8966 emits `[ecx + edx]`. hot_button_set's stores already
read the member each time and stay `[esi + eax]` (five store spellings tried).

**D16808 (game/unnamed_camera_0x4778b0.cpp). camera::update_orientation 0x477f60 matches** with each ring
step written `i = i - 1 < 0 ? 3 : i - 1;`. `if (--i < 0) i = 3;` (and `i--; if (i < 0)`) lets 8966 branch
on the flags of the dec (`jns`); `--i <= -1` gives `cmp -1`; the conditional expression gives the binary's
`dec / test / jge`.

**D16900 (game/unnamed_render_scene_0x481680.cpp; m-1020d worker 15; closer only). levelrender_push_attachment
0x483430: the inline budget.** The binary expands matrix::operator='s first two row copies with an out-of-line
vector3 copy constructor (0x410cd0) for each row's discarded operator= result and calls 0x413190 for the third;
8966 expanded all three while the depth was written per component. Copying the mesh's centre and radius into
locals and computing the depth through `vector3 d = center - eye; float depth = d.dot(fvec)` uses the budget as
the binary does: the function is now the binary's except that VC6 stores the radius copy (into the dead `item`
parameter slot) two instructions earlier, before the last faddp / fxch. The budget is spent in source order
(D9809), so expressions after a copy can still decide how that copy is expanded. Same value.

**D16901 (game/unnamed_tentacle_beast_0x4c6480.cpp; closer only). tentacle_beast_frame_hook 0x4c7960:** the same
inline-budget idea. `vector3 step = t->throw_velocity * reported_frametime` (the binary loads the frame time once,
`fld st(0)`), `lobj_ray ray; lobj_hit hit;` declared after the thrown test (lobj_ray's constructor stores
min_radius there, not before the first test), the landing as `r->pos = ray.dir * hit.t + r->pos`, the
miss as `r->pos += ray.dir`, and `vector3 fwd; fwd = r->orient.fvec;` (the binary's discarded out-of-line copy
of operator='s result). 170 -> 127 differing instructions; the first 60 match. Left: the binary keeps
&r->pos in EBP (pushed late) and calls vector3(x, y, z) and the copy constructor out of line on landing; a
`vector3 *` / reference local folds back to ESI + 0x10; frame 0xb4 against 0xa8. Same behaviour.

**D16902 (game/new_gamecreate.cpp; closer only). gamecreate_handle_click 0x409de0:** the closing Escape test is a
one-case `switch (key)` ( `mov ecx, key; mov eax, ecx; dec eax; je`, key returned from ECX); the tail
now matches. Left: the field-check block that the default case (key = 0) jumps to is laid out right after the
default by 8966; the binary places it after the typing path (all six orders of the three case labels, an if
chain, and the check written as `if (active) break; return 0;` compile the same).

**D16903 (notes only, m-1020d worker 15). Tried without result:** level_vis_reset (index forms CSE the constant
1, pointer forms give no bias, store orders), hud_drag_icon_begin (48 bit-field store orders and field types: the
binary ORs active onto the old value and masks with 1; 8966 folds all four into constants), level_vis_parse
(`set` at function scope / NULL-initialised: the zero register in EBP is not reproduced), levelrender_add_effect_entry
(13 store orders: POP ESI scheduling), StringPool::Add (36 declaration orders and 7 shapes: register choice),
hud_portrait_get_color (13 clamp forms; `if (v > 0) { if (v >= 255) v = 255; } else v = 0;` gives the outer
jbe but not the binary's dead `mov cl, al`), hud_hot_buttons_drag (dst / member orders; the binary loads the
hover slot into ECX and copies it to EBP), lobj_sphere_test_faces (12 forms of the inside flag; the binary's dead
second compare comes from a duplicated test block), gr_poly_clip_edge (i[p] / pointer-sum forms: the strength-
reduced address order does not follow the source), version_check_run (a goto into the SkipVerify tail is worse),
levelrender_draw_name_labels (a vector3 copy of the label position: frame +4 only; the binary has 44 bytes more).

## Behaviour review 2026-10-20 (D17000-)

Read-only reviews of the functions that do not byte-match or were never compared (eight agents, about 66 functions
each), every finding checked again in the listing before it was applied.

**D17000 (game/unnamed_shadows_0x4a37d0.cpp; review r4; behaviour fix in the decomp, corrects D16702).
obstacles_debug_render 0x4a3890: the normal tick is drawn through p0 / p1, and both keep the debug height.** In
the binary's frame (offsets with no pushes outstanding) p0 is 0x1c-0x24 and p1 0x10-0x18. The first line stores the
height into both y slots (`fst [esp+0x20]` / `fstp [esp+0x2c]` with three pushes, 0x4a3b55 / 0x4a3b59: 0x14 and
0x20). The tick then writes m.y to 0x24 (0x4a3bae, three pushes), t.x to 0x10 (0x4a3bd4, four), m.x to 0x1c
(0x4a3be5, five) and t.y to 0x18 (0x4a3be9, five), and passes 0x1c and 0x10 (0x4a3bed / 0x4a3bd8). So it runs from
(m.x, h, m.y) to (t.x, h, t.y). D16702 had read a third point `mid` with an unwritten y; that was p0. Without
`mid` the frame is the binary's size (D16702's 0xc bytes) and the instruction count matches (340 / 340); a register
choice still differs. Debug drawing only (Render_3d_obstacles).

**D17001 (game/unnamed_ai_move_0x4b1410.cpp; review r1; behaviour fix in the decomp). ai_turn_towards 0x4b2af0:
the degrees-per-radian constant is 57.295776f.** 0x4b2ec4 multiplies by the float at 0x580e24, 0x42652ee0
(57.2957763671875); `57.29578f` encodes as 0x42652ee1, one unit in the last place above it. It scales the turn
animation's speed. The same constant was already right in gr_3d (GR_RAD_TO_DEG) and gametarget.cpp.

**D17002 (game/level_scripts.cpp; review r8; behaviour fix in the decomp). savegame_init_conversation_slot
0x4d02d0: the topic bits are written back to the source slot i, not to `slot`.** Frame F = esp after the four
pushes: level F+0x2c, slot F+0x30, speakers F+0x34. The search loop keeps i in the dead `speakers` slot
(0x4d0404, 0x4d0431, 0x4d0441). The speaker copy reads F+0x30 (0x4d0469, no pushes outstanding): `slot`. The
topic copy reads `[esp+0x3c]` with two pushes outstanding (0x4d04e0): F+0x34, i. So
`set_topic_bit(level, i, ...)` stores each bit where it was just read, and only the in-memory topic flags take
the copied values. The decomp had `slot`. Game-bug candidate (not checked in game): a persistent speaker's topic
bits never reach the new slot's save data; docs/sum-exe/savegame.md updated.

**D17003 (game/level_scripts.cpp; review r8; behaviour fix in the decomp, answers NOTES-level-scripts open
question 1). savegame_two_stage_set 0x4d0e20, existing container entry: bit 0 = open, bit 1 = 0.** Frame O = esp
after the four pushes: open O+0x64, locked O+0x68, obj O+0x6c (reused for the shift, 0x4d1025). With the
printf's push outstanding, 0x4d1035 zeroes O+0x68 (locked) and 0x4d1043 reads O+0x64 (open); after `add esp, 4`
the fallback at 0x4d1050 reads O+0x68 (the zeroed locked). The stored value is `open ? 1 : locked`. The earlier
reading took 0x4d1043 for locked and stored 0 always. Written `(open ? 1 : (int)locked)` after `locked = false`.
The game-bugs.md entry ("stores both bits as 0") is withdrawn.

**D17004 (game/multi_chattracker.cpp; review r7; behaviour fix in the decomp). chat_process_irc_message 0x4ac240,
KICK: the user removed from the list is `nick`, the message's sender.** With the frame base B at 0x4acf11 (the
`add esp, 0x28` at 0x4acef4 balances the earlier pushes), the sprintf's first `%s` is `lea edx, [esp+0x78c]` with
two pushes outstanding, B+0x784, and the removal is `lea ecx, [esp+0x79c]` with six outstanding (0x4acf3a), also
B+0x784, the buffer JOIN passes to irc_add_user (0x4acd10). The kicked nick (prefix_copy) is B+0xe8c. The decomp
removed prefix_copy. Game-bug candidate (multiplayer chat, not checked): the kicker leaves the user list and the
kicked user stays.

**D17005 (game/unnamed_ai_think_0x4b5140.cpp; review r5; behaviour fix in the decomp). ai_choose_attack_slot
0x4b6a90: a new second-best slot takes best_dot as its dot.** best_dot is [esp+0x10], second_dot [esp+0x14] (the
first branch, 0x4b6b77-0x4b6b85, stores the dot into 0x10 and moves the old best's into 0x14). In the `else if`
branch 0x4b6b89 pops the dot with `fcomp` without storing it, and 0x4b6b94-0x4b6b9a copy [esp+0x10] into
[esp+0x14]: `second = i; second_dot = best_dot;`. The decomp stored dot. After that, only a dot above best_dot
moves `second` again, so `second` (and the sign of the slot fraction) can differ: dots 0.9, 0.5, 0.8 give
second = 1 in the binary, 2 in the old source. D9600 / D9810 had looked at precision only. Game-bug candidate
(probably a slip for `= dot`), not checked in game.

**D17006 (vsdk/unnamed_vfx_mesh_0x562a30.cpp; review r5; behaviour fix in the decomp).
vfx_mesh_instance::update_frame 0x563a40: a billboard's height is scaled by z, not y.** Skinned branch: scale is
at esp+0x4c/0x50/0x54 (vector3::assign into `lea ecx, [esp+0x54]` with two pushes, 0x563e94), rest_scale at
esp+0x24/0x28/0x2c; the width multiplies 0x4c and 0x24 (x), the height 0x54 and 0x2c (0x563fc4 / 0x563fc8, z).
Key-frame branch: the blended scale is at esp+0x24; the height multiplies 0x2c (0x56415f, z). The decomp used y
in both. Whether z was meant (a Z-up exporter) is not known; the binary's choice is kept.

**D17100 (game/unnamed_ai_groups_0x492b00.cpp; ai_group_think_caster 0x4936c0: six candidate records, not five).**
The first member loop (0x4937bc-0x4937d4) writes each record's character at `[eax-8]` from `lea eax, [esp+0x38]`,
so the records (boss_target, 0x10 bytes) start at esp+0x30 after the four pushes, and six end at esp+0x90, the
return address. Nothing else in the function reads or writes esp+0x80-0x8f. `boss_target list[6]` gives the
binary's `sub esp, 0x80` (list[5] gave 0x6c); the first difference moves from +0 to +0x1e (--score distance 321 ->
282). No behaviour change for a group of up to six. The "sixth member overwrites the return address" candidate in
docs/fixed-build.md came from reading the base as esp+0x38 and is withdrawn: an AI group of six fits. The overflow
that remains is C210's doubled count (fixed in SummonerFix and FIXED_BUILD).
