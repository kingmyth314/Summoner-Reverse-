// Layout checks for symbols/types.h: i686-w64-mingw32-gcc -fsyntax-only -I symbols symbols/types_check.c
// Layout checks for scratchpad types.h: sizeof of every touched or new struct, offsetof of every new field.
#include <stddef.h>
#include "types.h"
#define SZ(t, n) _Static_assert(sizeof(t) == (n), "sizeof " #t)
#define OF(t, f, n) _Static_assert(offsetof(t, f) == (n), #t "." #f)

SZ(sound_ref, 0xc);
OF(sound_ref, name, 0x0); OF(sound_ref, handle, 0x4); OF(sound_ref, params, 0x8);

SZ(ItemType, 0x100);
OF(ItemType, mesh, 0x10); OF(ItemType, animation, 0x14); OF(ItemType, value, 0x18);
OF(ItemType, stat_mods, 0x40); OF(ItemType, status_effects, 0x48); OF(ItemType, ap_regen, 0x54);
OF(ItemType, charge_effect, 0x58); OF(ItemType, flags, 0x5c); OF(ItemType, bitmap, 0x5e);
OF(ItemType, restricted, 0x67); OF(ItemType, damage, 0x68); OF(ItemType, weapon_flags, 0x6a);
OF(ItemType, weapon_skill, 0x6c); OF(ItemType, _unk7c, 0x7c); OF(ItemType, _unk80, 0x80);
OF(ItemType, render_distance, 0x84); OF(ItemType, weapon_class, 0x88); OF(ItemType, elemental_damage, 0x90);
OF(ItemType, weapon_triggered_effect, 0x94); OF(ItemType, armour_speed, 0xe8); OF(ItemType, protection, 0xf0);
OF(ItemType, _unkf4, 0xf4); OF(ItemType, armour_triggered_effect, 0xf8); OF(ItemType, description, 0xfc);

SZ(Player, 0x188);
OF(Player, char_info, 0x8); OF(Player, handle, 0xc); OF(Player, flags, 0x10); OF(Player, mp_owner, 0x14);
OF(Player, num_progression, 0x20); OF(Player, progression, 0x24); OF(Player, skill_cooldown, 0x28);
OF(Player, lent_max_hp, 0x178); OF(Player, skill_points, 0x17c); OF(Player, skill_cap, 0x180);
OF(Player, default_chain, 0x184);

SZ(spell_info, 0x80);
OF(spell_info, cast_effect, 0xc); OF(spell_info, cast_sound, 0x10); OF(spell_info, hit_effect, 0x1c);
OF(spell_info, hit_sound, 0x20); OF(spell_info, preloaded_hit_effect, 0x2c); OF(spell_info, misc_effect, 0x30);
OF(spell_info, misc_sound, 0x34); OF(spell_info, banish_effect, 0x40); OF(spell_info, _unk44, 0x44);
OF(spell_info, variation, 0x50); OF(spell_info, cost, 0x54); OF(spell_info, distance, 0x70);
OF(spell_info, flags, 0x74); OF(spell_info, damage_type, 0x78); OF(spell_info, target_radius, 0x7c);

SZ(char_info, 0x3d0);
OF(char_info, name, 0x0); OF(char_info, portrait, 0x4); OF(char_info, texture_effect, 0x8); OF(char_info, vfx, 0xc);
OF(char_info, species, 0x10); OF(char_info, checksum, 0x14); OF(char_info, anim_folder, 0x18);
OF(char_info, anim_class, 0x1c); OF(char_info, anim_class_mask, 0x20); OF(char_info, max_hp, 0x24);
OF(char_info, damage, 0x38); OF(char_info, special_ability, 0x3c); OF(char_info, special_arg, 0x3e);
OF(char_info, special_chance, 0x40); OF(char_info, reflex_ability, 0x44); OF(char_info, reflex_arg, 0x46);
OF(char_info, reflex_chance, 0x48); OF(char_info, moves, 0x4c); OF(char_info, team, 0x8c); OF(char_info, size, 0x90);
OF(char_info, skill, 0x94); OF(char_info, up_skill, 0xe8); OF(char_info, left_skill, 0xeb);
OF(char_info, alert_sound, 0xec); OF(char_info, _unkf8, 0xf8); OF(char_info, num_default_weapons, 0x114);
OF(char_info, default_weapons, 0x116); OF(char_info, default_weapon_materials, 0x11c); OF(char_info, prop_tag, 0x124);
OF(char_info, prop_tags, 0x128); OF(char_info, _unk168, 0x168); OF(char_info, _unk188, 0x188);
OF(char_info, num_footstep_sets, 0x19a); OF(char_info, _unk19c, 0x19c); OF(char_info, footstep_sounds_a, 0x1a0);
OF(char_info, footstep_sounds_b, 0x1dc); OF(char_info, skin, 0x218); OF(char_info, fov_range, 0x220);
OF(char_info, intelligence, 0x23c); OF(char_info, has_light, 0x240); OF(char_info, light_pos, 0x244);
OF(char_info, light_color, 0x248); OF(char_info, light_atten, 0x254); OF(char_info, flags, 0x258);
OF(char_info, icon_height, 0x264); OF(char_info, alt_textures, 0x268); OF(char_info, alt_index, 0x26c);
OF(char_info, alt_parent, 0x270); OF(char_info, num_drops, 0x274); OF(char_info, drop_items, 0x278);
OF(char_info, drop_chances, 0x2bc); OF(char_info, num_mp_drops, 0x300); OF(char_info, mp_drop_items, 0x304);
OF(char_info, mp_drop_chances, 0x348); OF(char_info, mp_drop_levels, 0x38c);

SZ(ai_state, 0x234);
OF(ai_state, pending_mode, 0x10); OF(ai_state, pending_submode, 0x14); OF(ai_state, combat_style, 0x18);
OF(ai_state, timer, 0x1c); OF(ai_state, _unk20, 0x20); OF(ai_state, timer30, 0x30); OF(ai_state, _unk34, 0x34);
OF(ai_state, priority, 0x38); OF(ai_state, timer3c, 0x3c); OF(ai_state, timer40, 0x40); OF(ai_state, timer44, 0x44);
OF(ai_state, _unk48, 0x48); OF(ai_state, owner, 0x4c); OF(ai_state, flags, 0x50); OF(ai_state, travel_dest, 0x54);
OF(ai_state, target, 0x60); OF(ai_state, prev_target, 0x64); OF(ai_state, _unk68, 0x68); OF(ai_state, _unk7c, 0x7c);
OF(ai_state, handle_slot, 0x80); OF(ai_state, mode_target, 0x84); OF(ai_state, _unk88, 0x88); OF(ai_state, goals, 0x8c);
OF(ai_state, cur_goal, 0x190); OF(ai_state, emerge_move, 0x194); OF(ai_state, _unk198, 0x198);
OF(ai_state, timer19c, 0x19c); OF(ai_state, disable_timer, 0x1a0); OF(ai_state, disable_ms, 0x1a4);
OF(ai_state, _unk1a8, 0x1a8); OF(ai_state, _unk1ac, 0x1ac); OF(ai_state, attackers, 0x1b0);
OF(ai_state, attackers[1].timer, 0x1bc); OF(ai_state, _unk1d8, 0x1d8); OF(ai_state, path_started, 0x1f8);
OF(ai_state, path_step, 0x200); OF(ai_state, _unk204, 0x204); OF(ai_state, perception_timer, 0x208);
OF(ai_state, path, 0x210); OF(ai_state, path_dest, 0x214); OF(ai_state, _unk220, 0x220);
OF(ai_state, timer224, 0x224); OF(ai_state, path_timer_a, 0x228); OF(ai_state, path_timer_b, 0x22c);
OF(ai_state, path_flags, 0x230);

SZ(sound_emitter, 0x74);
OF(sound_emitter, sound, 0x8); OF(sound_emitter, playing, 0xc); OF(sound_emitter, volume, 0x10);
OF(sound_emitter, pan, 0x14); OF(sound_emitter, volume_scale, 0x18); OF(sound_emitter, frequency_sent, 0x1c);
OF(sound_emitter, frequency, 0x20); OF(sound_emitter, pos, 0x24); OF(sound_emitter, last_pos, 0x30);
OF(sound_emitter, velocity, 0x3c); OF(sound_emitter, update_ms, 0x48); OF(sound_emitter, update_timer, 0x4c);
OF(sound_emitter, flags, 0x50); OF(sound_emitter, repeat_delay, 0x54); OF(sound_emitter, repeat_timer, 0x58);
OF(sound_emitter, seg_start, 0x5c); OF(sound_emitter, seg_end, 0x68);

SZ(ds_buffer, 0x250); OF(ds_buffer, acm_stream, 0x21c); OF(ds_buffer, num_refs, 0x24c);
SZ(ds_channel, 0x2c); OF(ds_channel, flags, 0x28);
SZ(snd_stream_slot, 0x1c); OF(snd_stream_slot, silence_bytes, 0x14); OF(snd_stream_slot, flags, 0x18);

SZ(script_action, 0x18);
OF(script_action, type, 0x0); OF(script_action, name, 0x4); OF(script_action, _unk08, 0x8);
OF(script_action, arg0c, 0xc); OF(script_action, arg10, 0x10); OF(script_action, flag14, 0x14);
SZ(script_object, 0x550);
OF(script_object, end_mode, 0x0); OF(script_object, actions, 0x4); OF(script_object, path, 0x514);
OF(script_object, timer, 0x518); OF(script_object, num_actions, 0x51c); OF(script_object, cur_action, 0x520);
OF(script_object, handle, 0x524); OF(script_object, suspended, 0x528); OF(script_object, hidden, 0x529);
OF(script_object, trigger_level, 0x52c); OF(script_object, trigger, 0x530);

SZ(quest_info, 0x6c);
OF(quest_info, name, 0x0); OF(quest_info, side_quest, 0x4); OF(quest_info, steps, 0x8);
OF(quest_info, steps[0].flag_hash, 0xc); OF(quest_info, num_steps, 0x68); OF(quest_info, invalidate_step, 0x69);

SZ(ui_element, 0x274);
OF(ui_element, initialized, 0x4); OF(ui_element, kind, 0x8); OF(ui_element, hidden, 0x1c); OF(ui_element, name, 0x1e);
OF(ui_element, id, 0x120); OF(ui_element, tooltips, 0x124); OF(ui_element, tooltips_prev, 0x128);
OF(ui_element, callback, 0x244); OF(ui_element, owned, 0x248); OF(ui_element, hotkeys, 0x24c);
OF(ui_element, gadget_type, 0x254); OF(ui_element, _unk258, 0x258); OF(ui_element, window, 0x25c);
OF(ui_element, window_prev, 0x260); OF(ui_element, window_next, 0x264); OF(ui_element, parent, 0x268);
OF(ui_element, next_sibling, 0x270);

SZ(game_camera, 0x2a8);
OF(game_camera, mode, 0x0); OF(game_camera, prev_mode, 0x4); OF(game_camera, high_view_suppressed, 0x8);
OF(game_camera, high_view_requested, 0x9); OF(game_camera, flags, 0xc); OF(game_camera, num_saved_states, 0x10);
OF(game_camera, saved_states, 0x14); OF(game_camera, zoom_high, 0x134); OF(game_camera, zoom_low, 0x138);
OF(game_camera, sequence, 0x13c); OF(game_camera, look_at, 0x140); OF(game_camera, target, 0x14c);
OF(game_camera, fixed_eye, 0x150); OF(game_camera, wanted_dir, 0x15c); OF(game_camera, orient_mode, 0x168);
OF(game_camera, default_distance, 0x16c); OF(game_camera, default_elevation, 0x170); OF(game_camera, default_zoom, 0x174);
OF(game_camera, default_look_height, 0x178); OF(game_camera, default_eye_offset, 0x17c); OF(game_camera, yaw, 0x180);
OF(game_camera, zoom, 0x184); OF(game_camera, elevation, 0x188); OF(game_camera, fov, 0x18c);
OF(game_camera, distance, 0x190); OF(game_camera, look_height, 0x194); OF(game_camera, eye_offset, 0x198);
OF(game_camera, eye_offset_wanted, 0x19c); OF(game_camera, saved_zoom, 0x1a0); OF(game_camera, eye_pos, 0x1a4);
OF(game_camera, eye_orient, 0x1b0); OF(game_camera, auto_view_timer, 0x1d4); OF(game_camera, enemy_near, 0x1d8);
OF(game_camera, dir_history, 0x1dc); OF(game_camera, dt_history, 0x20c); OF(game_camera, history_index, 0x21c);
OF(game_camera, history_count, 0x220); OF(game_camera, eye_velocity, 0x224); OF(game_camera, _unk230, 0x230);
OF(game_camera, height, 0x234); OF(game_camera, input_yaw, 0x23c); OF(game_camera, input_zoom, 0x240);
OF(game_camera, input_elevation, 0x244); OF(game_camera, seq_origin, 0x248); OF(game_camera, seq_orient, 0x254);
OF(game_camera, view_pos, 0x278); OF(game_camera, view_orient, 0x284);

SZ(effect_object, 0x10c);
OF(effect_object, vtable, 0x0); OF(effect_object, type, 0xc); OF(effect_object, pos, 0x10); OF(effect_object, orient, 0x38);
OF(effect_object, handle, 0x80); OF(effect_object, flags, 0x84); OF(effect_object, next, 0x98); OF(effect_object, prev, 0x9c);
OF(effect_object, instance, 0xa0); OF(effect_object, kind, 0xa4); OF(effect_object, spell, 0xa8); OF(effect_object, source, 0xac);
OF(effect_object, caster, 0xb0); OF(effect_object, owner, 0xb4); OF(effect_object, trail_bitmap, 0xb8); OF(effect_object, bits, 0xbc);
OF(effect_object, target, 0xc0); OF(effect_object, hit_handles, 0xc4); OF(effect_object, timer, 0xdc); OF(effect_object, sound, 0xe0);
OF(effect_object, radius, 0xe4); OF(effect_object, start, 0xe8); OF(effect_object, aim, 0xf4); OF(effect_object, avoid_area, 0x100);
OF(effect_object, timer2, 0x104); OF(effect_object, spell_effect, 0x108);

SZ(light, 0x94);
OF(light, flags, 0x8); OF(light, pos, 0xc); OF(light, spot_dir, 0x18); OF(light, spot_angle, 0x24);
OF(light, radius_inner, 0x2c); OF(light, radius_outer, 0x30); OF(light, color, 0x34); OF(light, local_pos, 0x40);
OF(light, local_dir, 0x4c); OF(light, radius_inner_sq, 0x58); OF(light, radius_outer_sq, 0x5c); OF(light, spot_neg_cos, 0x60);
OF(light, bitmap, 0x68); OF(light, proj_orient, 0x6c); OF(light, sort_dist, 0x90);

SZ(shop_item, 0x8); OF(shop_item, item_type, 0x0); OF(shop_item, price, 0x4);
SZ(shopkeeper, 0x14);
OF(shopkeeper, name, 0x0); OF(shopkeeper, num_sell, 0x4); OF(shopkeeper, sell, 0x8); OF(shopkeeper, num_buy, 0xc); OF(shopkeeper, buy, 0x10);

SZ(container, 0xe8);
OF(container, items, 0x98); OF(container, num_items, 0xd8); OF(container, user, 0xdc);
OF(container, mesh_item_type, 0xe0); OF(container, lock, 0xe4);

SZ(cutscene_entry, 0x140);
OF(cutscene_entry, name, 0x0); OF(cutscene_entry, flags, 0x4); OF(cutscene_entry, level, 0x8); OF(cutscene_entry, script, 0xc);
OF(cutscene_entry, num_cameras, 0x10); OF(cutscene_entry, cameras, 0x14); OF(cutscene_entry, num_effects, 0x24);
OF(cutscene_entry, effects, 0x28); OF(cutscene_entry, num_shot_effects, 0x30); OF(cutscene_entry, shot_effects, 0x34);
OF(cutscene_entry, soundtrack, 0x5c); OF(cutscene_entry, num_minitracks, 0x60); OF(cutscene_entry, minitracks, 0x64);
OF(cutscene_entry, num_fogs, 0x74); OF(cutscene_entry, fogs, 0x78); OF(cutscene_entry, num_vissets, 0xb8);
OF(cutscene_entry, vissets, 0xbc); OF(cutscene_entry, num_fades, 0xfc); OF(cutscene_entry, fades, 0x100);

SZ(cutscene_camera_shot, 0x70);
OF(cutscene_camera_shot, vtable, 0x0); OF(cutscene_camera_shot, state, 0x4); OF(cutscene_camera_shot, fov, 0x8);
OF(cutscene_camera_shot, start, 0xc); OF(cutscene_camera_shot, end, 0x10); OF(cutscene_camera_shot, track, 0x14);
OF(cutscene_camera_shot, pos, 0x40); OF(cutscene_camera_shot, orient, 0x4c);

SZ(worldmap_location, 0x2c);
OF(worldmap_location, flags, 0x0); OF(worldmap_location, navpoint, 0x4); OF(worldmap_location, arrow_scale, 0x8);
OF(worldmap_location, arrow_angle, 0xc); OF(worldmap_location, arrow_x, 0x10); OF(worldmap_location, arrow_y, 0x14);
OF(worldmap_location, label_x, 0x18); OF(worldmap_location, label_y, 0x1c); OF(worldmap_location, label_w, 0x20);
OF(worldmap_location, label_h, 0x24); OF(worldmap_location, label, 0x28);

SZ(decal, 0x128);
OF(decal, timer, 0x0); OF(decal, bitmap, 0x4); OF(decal, pos, 0x8); OF(decal, radius, 0x14); OF(decal, normal, 0x18);
OF(decal, flags, 0x24); OF(decal, next, 0x28); OF(decal, prev, 0x2c); OF(decal, arg30, 0x30); OF(decal, plane_scale, 0x34);
OF(decal, projector, 0x38); OF(decal, u_axis, 0x118); OF(decal, v_axis, 0x11c); OF(decal, u_scale, 0x120); OF(decal, v_scale, 0x124);

SZ(lamp_glow, 0x18);
OF(lamp_glow, pos, 0x0); OF(lamp_glow, size, 0xc); OF(lamp_glow, alpha, 0x10); OF(lamp_glow, _unk14, 0x14);

SZ(boss_threat, 0x18);
OF(boss_threat, handle, 0x0); OF(boss_threat, threat, 0x4); OF(boss_threat, timer, 0x14);

#define A(f, o) _Static_assert(offsetof(Character, f) == (o), #f)
A(obj_prev, 0x8); A(last_pos, 0x1c); A(start_navpoint, 0x34); A(orient, 0x38); A(last_orient, 0x5c);
A(bound_radius, 0x88); A(net_handle, 0x8c); A(trigger_latch, 0xa0); A(death_move_sub, 0xa4); A(death_move, 0xa8);
A(ai, 0x324); A(combat, 0x558);
A(anim_stand, 0x5ac); A(anim_norm_walk, 0x5b0); A(anim_norm_run, 0x5b4); A(anim_ready, 0x5b8); A(anim_walk, 0x5bc);
A(anim_run, 0x5c0); A(anim_stand_hurt, 0x5c4); A(anim_norm_walk_hurt, 0x5c8); A(anim_norm_run_hurt, 0x5cc);
A(anim_ready_hurt, 0x5d0); A(anim_walk_hurt, 0x5d4); A(anim_run_hurt, 0x5d8); A(anim_sneak_ready, 0x5dc);
A(anim_hide_ready, 0x5e0); A(anim_tripped_liedown, 0x5e4); A(anim_cloaked_ready, 0x5e8); A(anim_cloaked_sneak, 0x5ec);
A(anim_statscreen, 0x5f0); A(base_max_hp, 0x5f4);
A(hit_by_party, 0x608); A(move_fx, 0x60c); A(frametime, 0x610); A(skill, 0x614);
A(move_sound, 0x674); A(foot_tag_l, 0x678); A(foot_tag_r, 0x67a); A(ground_hit, 0x67c); A(ground_lobj, 0x688);
A(ground_face, 0x68a); A(layer, 0x68c); A(stance_blend, 0x694); A(forward_speed, 0x698); A(velocity, 0x69c);
A(loco_blend, 0x6a8); A(target_speed, 0x6ac); A(slowdown_rate, 0x6b0); A(run_speed, 0x6b4); A(walk_speed, 0x6b8);
A(speed_scale, 0x6bc); A(cur_anim, 0x6cc); A(timer6d4, 0x6d4); A(cutscene_object, 0x6d8); A(script_object, 0x6dc);
A(mount_link, 0x6e0); A(hp_overflow, 0x6e8); A(pos_mod_text, 0x6ec); A(hp_text_timer, 0x6f0); A(pos_mod_text_timer, 0x6f4);
A(float_text, 0x6f8); A(float_text_nul, 0x708); A(float_text_rgb, 0x709); A(float_text_scale, 0x70c);
A(move_turn_angle, 0x714); A(move_turn_applied, 0x718); A(info, 0x71c); A(prop_tag, 0x720); A(default_prop_tag, 0x724);
A(num_attachments, 0x728); A(attachments, 0x72c); A(equipment, 0x754);
/* known fields unchanged */
A(type, 0xc); A(pos, 0x10); A(centre_offset, 0x28); A(handle, 0x80); A(obj_flags, 0x84); A(team, 0x98); A(move_start_time, 0x9c);
A(status_slots, 0xac); A(timer2ec, 0x2ec); A(enemy_team_mask, 0x2f0); A(name, 0x2f4); A(instance, 0x314); A(live_next, 0x318);
A(live_prev, 0x31c); A(flags, 0x320); A(hp, 0x5f8); A(base_max_ap, 0x5fc); A(ap, 0x5fe); A(level, 0x600); A(xp, 0x604);
A(cur_move, 0x6d0); A(mount_flags, 0x6e4);
_Static_assert(sizeof(((Character*)0)->attachments[0]) == 8, "attachment");
_Static_assert(sizeof(Character) == 0x788, "sizeof Character");
_Static_assert(sizeof(ds_buffer)==0x250,"b");
_Static_assert(sizeof(ds_channel)==0x2c,"c");
_Static_assert(offsetof(ds_channel,flags)==0x28,"f");
_Static_assert(offsetof(ds_buffer,num_refs)==0x24c,"n");
_Static_assert(sizeof(snd_stream_slot)==0x1c,"s");
_Static_assert(offsetof(ai_state,owner)==0x4c,"o");
_Static_assert(offsetof(ai_state,travel_dest)==0x54,"t");
SZ(mp_packet_info, 0x1c);
OF(mp_packet_info, handler, 0x4); OF(mp_packet_info, flag, 0x8); OF(mp_packet_info, sent, 0xc);
OF(mp_packet_info, name, 0x10); OF(mp_packet_info, rate_flags, 0x18);
SZ(mp_player, 0xbc);
OF(mp_player, addr, 0x8); OF(mp_player, flags, 0x10); OF(mp_player, type, 0x14); OF(mp_player, socket, 0x18);
OF(mp_player, id, 0x1c); OF(mp_player, join_timer, 0x24); OF(mp_player, object, 0x28); OF(mp_player, char_info, 0x2c);
OF(mp_player, profile_flags, 0x30); OF(mp_player, name, 0x34); OF(mp_player, display_name, 0x54);
OF(mp_player, move_update_timer, 0x64); OF(mp_player, swi_timer, 0x68); OF(mp_player, _unk6c, 0x6c);
SZ(mp_netgame_info, 0x6c);
OF(mp_netgame_info, password, 0x20); OF(mp_netgame_info, level, 0x30); OF(mp_netgame_info, flags, 0x38);
OF(mp_netgame_info, num_players, 0x3c); OF(mp_netgame_info, difficulty, 0x44); OF(mp_netgame_info, state, 0x54);
OF(mp_netgame_info, server_addr, 0x5c); OF(mp_netgame_info, server_player_id, 0x64); OF(mp_netgame_info, level_scale, 0x68);

/* 2026-10-15 notes sweep */
_Static_assert(sizeof(console_command) == 0x0C, "console_command is 0xC on PC");
_Static_assert(offsetof(ItemType, mesh) == 0x10, "ItemType.mesh");
_Static_assert(offsetof(ItemType, damage) == 0x68, "ItemType.damage");
_Static_assert(sizeof(char_info) == 0x3D0, "char_info size");
_Static_assert(offsetof(char_info, damage) == 0x38, "char_info.damage");
_Static_assert(sizeof(shop_item) == 0x08, "shop_item size");
_Static_assert(sizeof(shopkeeper) == 0x14, "shopkeeper size");
_Static_assert(offsetof(ui_element, callback) == 0x244, "ui_element.callback");
