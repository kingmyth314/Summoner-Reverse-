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
declared. console_command: 0x10 bytes (the objects are 0x10 apart in .bss) with types.h's three fields first.

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

## Checked

- Every header compiles on its own, and all headers compile together in one translation unit (no
  conflicting overloads among the 730 free functions).
- Every pilot .cpp passes `-fsyntax-only -Wall` with only the warnings it had before (new_status.cpp 1,
  character_instance.cpp 4, window.cpp 1), and `-Wmissing-prototypes` finds no definition without a
  matching header declaration except the unreferenced `ui_stub_ret_52afe0` (now declared in window.h).
