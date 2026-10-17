# match21 notes (cl 12.00.8966, VC6 SP5)

Tree: copy of snapL/src/decomp. Game units on their /O2 lines; vsdk units on the coordinator's /Ox /Ow lines
(vc6-flags.vsdk-ow.txt, D7502). "Before" = the snapL source under the same flags.

## Per file, before -> after

| file | before | after | won |
|---|---|---|---|
| game/new_pxochat.cpp | 39/57 | 41/57 | 0x423530 version_check_frame, 0x4236c0 login_frame |
| game/unnamed_goals_doors_0x475180.cpp | 52/67 | 55/67 | 0x475fb0 ai_goal_type_fields, 0x476300 goal_remove, 0x476940 door_is_open |
| game/unnamed_camera_0x4778b0.cpp | 35/47 | 38/47 | 0x477ea0 compute_eye_pos, 0x477f10 get_target_offset, 0x479a60 cycle_mode |
| game/pathfinding_visibility.cpp | 3/11 | 4/11 | 0x4b7da0 pathvis_edge_pool_init |
| game/unnamed_mp_objects_0x47f860.cpp | 18/31 | 18/31 | - |
| game/unnamed_ai_think_0x4b5140.cpp | 19/29 | 19/29 | - |
| game/gametarget.cpp | 12/22 | 12/22 | - |
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 14/24 | 19/24 | 0x545570 put_data, 0x5455a0 uv, 0x545760 alpha, 0x5459d0 clip_plane, 0x5460e0 get_data |
| vsdk/unnamed_vif_0x5329f0.cpp | 5/15 | 8/15 | 0x532e80 records_reset, 0x532ea0 reset_all, 0x5339b0 record_release |
| vsdk/unnamed_ix_0x53ac20.cpp | 24/35 | 24/35 | - |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 29/39 | 29/39 | - |
| vsdk/unnamed_vfx_emitter_0x55ff00.cpp | 5/14 | 5/14 | - |

Game 178 -> 187 of 274 (+9); vsdk under /Ox /Ow 77 -> 85 of 127 (+8). Against the old /O2 vsdk flags the
vsdk files were 74/127 (gr_poly 12, vfx_emitter 4: put_float, put_vector and get_color come from the flags alone).
None lost, checked address by address (final.txt against base.txt). mingw -fsyntax-only: 0 errors in every
changed file. No header changed.

## Finding: the vsdk was built with /Ow (D8000)

Before the coordinator's message I measured the same thing on this tree: /Ow on every vsdk line (/O2 base) takes
the 92 vsdk units from 1,593 to 1,683 matches (+92, lost 2: gr_glide 0x549c80 and unnamed_net 0x517780, both
written for the no-/Ow code); /Oa gives 1,586; /Ow on the game units loses (2,914 -> 2,584). Under /Ow the plain
gr_poly forms (memcpy plus `+= n`, direct `cursor->pass[index]` stores, `cursor++`) are the ones that match, so
D7001's local-copy shapes and D7006's "dead argument slot" explanation for gr_poly were artefacts of the missing
flag. put_byte / put_int are back to the plain `*gr_poly_buffer_ptr++ = b` / `+= 4` forms (still match).

## What worked (D8001)

- local declaration order for initialised locals (version_check_frame), switch cases in block order (login_frame);
- select-a-constant-then-one-call: `int m; if .. m = 1; else if .. m = 2; ...; set_mode(m);` (cycle_mode);
- switch fall-through for nested range tests (ai_goal_type_fields);
- one shared `return false` (door_is_open);
- direct member-array indexing instead of an element pointer (goal_remove, edge_pool_init with an index loop);
- returned vector3 through a field-filled local (compute_eye_pos, get_target_offset);
- index loops (vif records), plist_append macro (vif_record_release), `*dst = t; ...; dst++` (clip_plane).

## Still differs (D8002), main causes

- pxochat: page_down clamp polarity; the three version popups (common-store hoisting); init / add_chat_line
  clamp register (`and eax, edx`); update_scrollbar / draw_* / drag_scrollbar register and load scheduling;
  handle_click, button_action, build_popup_text, add_typed_channel: larger layout differences not reached.
- goals_doors: pack_orientation (a vector2 copy in the dead argument slots); goal_find (pointer base into the
  goal and `test` operand order); evaluate / alloc / process_goals / door_* larger.
- camera: update_eye_position (`zoom * elevation` always elevation first); the rest larger.
- mp_objects: D5812 / D7203 handler `mov eax, [eax]`, ai_disable's `mov cx / and ecx, 0xffff`.
- vfx_emitter: get_fade (frame setup scheduled first), init_particle (frame 0xdc vs 0xe8, `fild / faddp`).
- vif: curve__evaluate is close (the count byte compared before n is formed: applied `if (c->count == 1)
  ... n = c->count;`; the binary also pushes all three registers before the first test).
- pathvis_add_edge_node: the list_remove macro (D7006), a header question left alone.
- ix, gr_3d, ai_think, gametarget: no near misses found in the time; register / frame layout.

## Logic errors / game bugs

None new. (Existing notes stand: matrix3_pack_orientation stores rvec twice; vif_curve__evaluate's reciprocal.)

## Note on Confidence

Several gr_poly functions were already marked "exact" without a VC6 match under 8966 (set_texture, rgb is
"likely"); set_texture 0x545340 still carries "exact" from before and does not match under /Ox /Ow; left as it
was, flagged here for the coordinator.
