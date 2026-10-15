# decomp66: Sum.exe v1.40, 0x5565e0-0x56c4d0

Written from the Sum.exe disassembly (tools/asm.mjs) against the src/decomp headers of a7d9e66 / 5b92510
(the include copy was rebased onto every commit up to 5b92510). Every file is a placeholder
(`vsdk/unnamed_<what>_<address>.cpp`): no unit in this range has a path string, a strings.tbl block or a
Summoner 2 counterpart with the same functions in the same order, so modules.txt is empty. Decisions:
D4600-D4612 (include/DECISIONS.md). Header changes: CHANGED-HEADERS.txt.

Status: 18 new unit files (about 350 functions and COMDATs annotated: 252 exact, 97 likely, 4 unsure, from
the per-function comments), one modified repo file (vsdk/unnamed_snd_0x5204c0.cpp, D4603). Not written: the
vfx mesh object 0x562a30-0x564d62 (39 functions, list below). libjpeg is listed, not rewritten.

## Units (in address order)

| Start | End | File (placeholder) | Evidence |
|---|---|---|---|
| 0x5565e0 | 0x5566dc | unnamed_jpeg_write_0x5565e0.cpp (jpeg_write_file) | after targautils.cpp (ends 0x5565d6, .data ".pcx" 0x5adb88); its only literal "wb" is the next .data word 0x5adb90; no .bss; may be the head of the String file |
| 0x5566e0 | 0x558300 | unnamed_string_0x5566e0.cpp (String, TextBuffer) | .data 0x5adb94-0x5adbe7 (TextBuffer's two initialised globals first, then the String literals "%d" .. "0x%x"), .bss 0x2f53af0-0x2f53aff (operator[]'s spare char, three "" literals) |
| 0x558340 | 0x558a23 | unnamed_linefile_0x558340.cpp (LineFile) | .data 0x5adbe8-0x5adbfb (LineFile_eol and "\r\n", "rb", "ab", "wb"); no .bss |
| 0x558a30 | 0x5590a2 | unnamed_bezier_0x558a30.cpp (bezier4, bezier, spline) | between LineFile and the clip unit's initialiser; no data |
| 0x5590b0 | 0x559da7 | unnamed_clip_0x5590b0.cpp (gr clipper, bbox_*) | initialiser table entry 648 (0x5590b0, the 64 temporary vertices); .bss 0x2f53b00-0x2f54703; closing COMDAT vector3::operator[] 0x559da0 |
| 0x559db0 | 0x559e9b | unnamed_lfsr_0x559db0.cpp (lfsr_cipher) | after the clip unit's COMDAT; no data; its constructor is the COMDAT 0x513eb0 of the vfile unit |
| 0x559ea0 | 0x55a8db | unnamed_ftp_0x559ea0.cpp (ftp_get) | .data 0x5adbfc-0x5adc7f; no .bss |
| 0x55a8e0 | 0x55b431 | unnamed_http_0x55a8e0.cpp (http_get, async lookup) | .data 0x5adc80-0x5add23 (no pooling with the FTP literals), .bss 0x2f54708-0x2f54717 |
| 0x55b440 | 0x55bd77 | unnamed_vfx_channel_0x55b440.cpp (forces, key channels) | no data; built without inline expansion |
| 0x55bd80 | 0x55bf88 | unnamed_bitarray_0x55bd80.cpp (BitArray) | no data; whether it is the tail of the channel object cannot be told |
| 0x55bf90 | 0x55dda6 | unnamed_ds_0x55bf90.cpp (ds_*) | initialiser entry 649 (the channel array); .data 0x5add24-0x5ae0b7, .bss 0x2f54720-0x2f8f743; closing COMDAT ds_channel's constructor 0x55dda0 |
| 0x55ddb0 | 0x55e03f | unnamed_ds3d_0x55ddb0.cpp (ds3d_*) | own .data 0x5ae0b8-0x5ae103 (the EAX 1.0 reverb GUID, the message) and .bss 0x2f8f744-0x2f8f749 after the ds object's COMDAT |
| 0x55e040 | 0x55f10b | unnamed_ui_slider_0x55e040.cpp (bevels, ui_slider_base, ui_slider, ui_tooltip) | no data of its own |
| 0x55f110 | 0x55fc6f | unnamed_poly_split_0x55f110.cpp (gr_poly_split_*, gr_poly_get_section) | .bss 0x2f8f750-0x2f91a5b |
| 0x55fc70 | 0x55fefa | unnamed_vfx_dummy_0x55fc70.cpp (fx_rec2c, vfx_dummy_desc::relocate) | no data; may be the head of the emitter object |
| 0x55ff00 | 0x562a2b | unnamed_vfx_emitter_0x55ff00.cpp (emitters, particles) | initialiser entry 650 (gr_mode_vfx_flat_alpha, .bss 0x2f91a5c); COMDAT block 0x5622b0-0x562a2b |
| 0x562a30 | 0x564d62 | (not written: the vfx mesh object) | initialiser entry 651 (the dump_mesh_frames ccmd 0x2f91a60), .bss 0x2f91a60-0x2f91a6f (Dump_mesh_frames 0x2f91a6c), .data 0x5ae104-0x5ae1db; COMDAT block 0x564770-0x564d62 (D4611) |
| 0x564d70 | 0x56565c | unnamed_vfx_material_0x564d70.cpp (vfx_tex_layer, vfx_material, vfx_base_tag::relocate, vfx_camera_desc) | no initialiser, no .data; "" literal 0x2f91a70; possibly three objects (D4611) |
| 0x565660 | 0x56655f | libjpeg (IJG 6b), first run | IJG names; see below |
| 0x566560 | 0x56742b | unnamed_sndstream_0x566560.cpp (snd_stream_*, acm_*, wav_*) | between the two libjpeg runs; .data 0x5af494-0x5af4b3 ("????", "MS-ADPCM", "msacm32.dll") after jerror.c's literals; .bss 0x2f91a78-0x2fbdb7f |
| 0x567430 | 0x56c4cf (continues to 0x56c732) | libjpeg (IJG 6b), second run | IJG names; jfdctflt.c (jpeg_fdct_float 0x56c4d0) starts at the range end |

The initialiser table at 0x588004 has no user entry after 651 (0x562a30): 652 and 653 are 0, then the CRT's.

## libjpeg (third-party, not rewritten)

IJG release 6b: jpeg_CreateCompress checks `version == 0x3e` (62, JPEG_LIB_VERSION of 6b) and the structure size
0x178 (0x56566c / 0x565691), and jpeg_write_m_header / jpeg_write_m_byte (added in 6b) are present. Only the
float DCT (forward_DCT_float, jfdctflt.c) and neither jcphuff.c (progressive) nor jcmaster.c's validate_script
(multi-scan) are linked: the library was built with DCT_FLOAT only and without C_PROGRESSIVE_SUPPORTED /
C_MULTISCAN_FILES_SUPPORTED. The game calls it from jpeg_write_file (0x5565e0) only. Files by address (names only):

- 0x565660-0x5659bf jcapimin.c: jpeg_CreateCompress, jpeg_destroy_compress, jpeg_abort_compress, jpeg_suppress_tables, jpeg_finish_compress, jpeg_write_marker, jpeg_write_m_header, jpeg_write_m_byte, jpeg_write_tables
- 0x5659c0-0x565baf jcapistd.c: jpeg_start_compress, jpeg_write_scanlines, jpeg_write_raw_data
- 0x565bb0-0x56629f jcparam.c: jpeg_add_quant_table, jpeg_set_linear_quality, jpeg_quality_scaling, jpeg_set_quality, jpeg_set_defaults, std_huff_tables, add_huff_table, jpeg_default_colorspace, jpeg_set_colorspace
- 0x5662a0-0x5663af jdatadst.c: jpeg_stdio_dest, init_destination, empty_output_buffer, term_destination
- 0x5663b0-0x56655f jerror.c: jpeg_std_error, error_exit, output_message, emit_message, format_message, reset_error_mgr
- (0x566560-0x56742b: the game's sound-stream code)
- 0x567430-0x56803f jmemmgr.c: jinit_memory_mgr, alloc_small, out_of_memory, alloc_large, alloc_sarray, alloc_barray, request_virt_sarray, request_virt_barray, realize_virt_arrays, access_virt_sarray, do_sarray_io, access_virt_barray, do_barray_io, free_pool, self_destruct
- 0x568040-0x5680df jcomapi.c: jpeg_abort, jpeg_destroy, jpeg_alloc_quant_table, jpeg_alloc_huff_table
- 0x5680e0-0x5689cf jcmarker.c: jinit_marker_writer, write_marker_header, emit_marker, emit_byte, emit_2bytes, write_marker_byte, write_file_header, emit_jfif_app0, emit_adobe_app14, write_frame_header, emit_dqt, emit_sof, write_scan_header, emit_dht, emit_dac, emit_dri, emit_sos, write_file_trailer, write_tables_only
- 0x5689d0-0x568a8f jcinit.c: jinit_compress_master
- 0x568a90-0x568b1f jmemnobs.c: jpeg_get_small, jpeg_free_small, jpeg_get_large, jpeg_free_large, jpeg_mem_available, jpeg_open_backing_store, jpeg_mem_init, jpeg_mem_term
- 0x568b20-0x568bef jutils.c: jdiv_round_up, jround_up, jcopy_sample_rows, jcopy_block_row, jzero_far
- 0x568bf0-0x568d7f jcmainct.c: jinit_c_main_controller, start_pass_main, process_data_simple_main
- 0x568d80-0x5695cf jccoefct.c: jinit_c_coef_controller, start_pass_coef, start_iMCU_row, compress_data, compress_first_pass, compress_output
- 0x5695d0-0x56a35f jchuff.c: jpeg_make_c_derived_tbl, jpeg_gen_optimal_table, jinit_huff_encoder, start_pass_huff, encode_mcu_huff, encode_one_block, emit_bits, dump_buffer, emit_restart, flush_bits, finish_pass_huff, encode_mcu_gather, htest_one_block, finish_pass_gather
- 0x56a360-0x56a6af jcdctmgr.c: jinit_forward_dct, start_pass_fdctmgr, forward_DCT_float
- 0x56a6b0-0x56ac6f jcprepct.c: jinit_c_prep_controller, start_pass_prep, pre_process_data, expand_bottom_edge, pre_process_context, create_context_buffer
- 0x56ac70-0x56b65f jcsample.c: jinit_downsampler, start_pass_downsample, sep_downsample, int_downsample, expand_right_edge, fullsize_downsample, h2v1_downsample, h2v2_downsample, h2v2_smooth_downsample, fullsize_smooth_downsample
- 0x56b660-0x56bd9f jccolor.c: jinit_color_converter, rgb_ycc_start, rgb_ycc_convert, rgb_gray_convert, cmyk_ycck_convert, grayscale_convert, null_convert, null_method
- 0x56bda0-0x56c4cf jcmaster.c: jinit_c_master_control, initial_setup, prepare_for_pass, select_scan_parameters, per_scan_setup, pass_startup, finish_pass_master
- 0x56c4d0- jfdctflt.c: jpeg_fdct_float (beyond this range)

The order inside jcmarker.c (jinit_marker_writer first) and of the files is the link order, not the IJG
source order. The declarations the game needs are in include/vsdk/bmpman/jpeg_min.h (extern "C").

## Not written

The vfx mesh object 0x562a30-0x564d62 (39 functions): 0x562a30 init_ccmd_dump_mesh_frames_thunk, 0x562a40
init_ccmd_dump_mesh_frames, 0x562a60 cmd_dump_mesh_frames, 0x562b20 fx_rec98::init, 0x562b90 vfx_stub_ret10,
0x562ba0 fx_rec98::render, 0x562ca0 render_billboards, 0x562f60 vfx_mesh_render, 0x563a40 update_frame, 0x5645b0
vfx_node_desc::relocate, 0x564720 vfx_node_desc::load_bitmap, and the inline members 0x564770-0x564d30
(vfx_mesh_xform / morph / anim / face_uv_anim relocate, vfx_ptr0_relocate, is_billboard, face get_u / get_v /
set_u / set_v, float_round_to_int, vfx_unpack_normal, vfx_pack_normal, vfx_unpack_vector_s8, xform get_pos /
get_scale / get_rot, morph get_pos, get_vertex, anim get_a / get_b / get_vec, vfx_get_fixed8_vector_4,
face_uv_anim get_u / get_v, frame_bit, has_flag4, vfx_ptr4_relocate); the globals ccmd_dump_mesh_frames
(0x2f91a60) and Dump_mesh_frames (0x2f91a6c). Written elsewhere: ptr_relocate 0x564d50 and the particle
accessors 0x564c60 / 0x564c70 (inline, vfx.h / effect_mem.h). What was read, for the follow-up:
vfx_node_desc (0xa8): +0x08 base, +0x0c word (bits 6-15 frame rate, bit 1 a skinned anim_track at +0x6c,
bit 2 vertices * 64, bit 3 xform positions / 16, bit 4 static vertices), +0x0e frame count, +0x10 vertex
count, +0x12 face-uv-animation count (0x48 each at +0x58), +0x16 count of 0xc records at +0x40, +0x18 flags
(0x1 sprite, 0x800 line quad: is_billboard tests 0x801, bit 2 has_flag4), +0x30 start frame, +0x38 / +0x3c
(material index bytes) / +0x40 / +0x50 (morph) / +0x54 (anim) / +0x5c (vertices) / +0x60 / +0x64 / +0x68
(xform) / +0x6c pointers, +0x44 BitArray of shown frames, +0x70 / +0x7c / +0x84 the rest pose, +0x94 glow
bitmap name (reloc_ptr) / +0x98 its handle. fx_rec98: +0x0c / +0x10 the sprite width and height, +0x14 alpha,
init sets hidden (bit 31) and clears bound / placed; render updates the frame only when the instance is not
vfx_last_mesh_instance. Output vertices go to vfx_mesh_verts (0x272b900).

## Function list (address, name, confidence; from the per-function comments)

See the end of this file.

## Documentation and symbol disagreements

- sum.symbols: the four fx_rec28 position / velocity accessors are swapped (D4607); vfx_particle__get_size /
  set_size are the particle angle; vfx_emitter__get_fixed12_8c .. 94 are members of the instance (fx_recb0),
  not of the emitter descriptor; vfx_particle_get_fade / get_color / get_bitmaps / get_billboard_axes are
  thiscall members of the instance.
- sum.symbols vfx_force__get_transform (0x55b730) is the force state's (fx_rec4c) member.
- sum.symbols vfx_tex_layer__ctor (0x564ef0) zeroes 0x18 bytes: not the 0x14-byte texture layer.
- sum.symbols ds_find_free_buffer (0x55ce30) is the buffer-name hash used by ds_find_buffer / ds_load_buffer.
- The unit table of the docs (0x562a30-0x565610 as one unit) splits at 0x564d62 (D4611).
- decomp67's vfx_force_desc and decomp66's vfx_force were the same record (D4606).
- gr_bitmap_3d's second argument was declared int (D4609); ds_play_streamed's third int (D4603).

## Game bugs

- Effect dummies bound to a bone never take the bone's rotation: fx_rec2c::update builds the product matrix and
  then overwrites it with q14.to_matrix (0x55febc) instead of reading the quaternion back (the emitter does,
  0x56164f).
- Emitter force integration (move_particles, 0x560c16 and the three copies): the sub-step loop advances its
  counter by 0.5 s while each step is at most 1/30 s, so a frame longer than 1/30 s moves the particles under
  forces by only 1/30 s.
- render_d3d (unused): particles in the top 100 screen pixels are culled (y is tested against +100, x against
  -100; 0x561bd6), and rhw is set to the view z instead of 1/z (0x561de6).
- get_billboard_axes computes the tip with the size factor and overwrites it at once without it (0x5629ad /
  0x562a04): flat particles' length does not scale with their size.
- jpeg_write_file's row table holds 1024 rows: a screenshot taller than 1024 lines overruns the stack array.
- ds_channel_update_fade: a fading sound that ends by itself keeps the fading flag, so ds_channel_release never
  frees the channel and the next sound on it inherits the fade.
- ftp_get::abort_get spins forever when the worker never sets done (a failed connect or login).
- http_get: a failed asynchronous host lookup is never seen (http_resolve_host marks its own record, the poll
  reads the global copy), so an unknown host polls until the getter is aborted.

## Open questions

- 0x2f8f754 (inside the polygon split unit's .bss, between the free list and the pool) has no reference.
- ds_pause_all tests its argument as a byte (a bool in the original; kept int as declared).
- Whether the dummy code is the head of the emitter object, BitArray the tail of the channel object,
  jpeg_write_file the head of the String file, and the material code one or three objects.
- The dummy and camera descriptors call vfx_light_track::get_position's COMDAT 0x5202f0 (dummies) and
  vfx_node::get_orient: written as identical-code folding and casts (D4606); the original may have had a
  common base class.
- gr_vertex has no default constructor in the headers (D1605): the emitter's flat path runs gr::Vertex's
  0x5081c0 on three locals in the original.

## Function list

One line per file; entries that cover several COMDATs are listed once. (see file): operators and records whose name the comment does not repeat.

- unnamed_bezier_0x558a30.cpp: 0x558a30 bezier_bernstein3 exact; 0x558ab0 bezier4::bezier4 exact; 0x558af0 bezier4::bezier4 exact; 0x558b80 bezier4::eval exact; 0x558be0 factorial exact; 0x558c00 bezier::bezier exact; 0x558c50 bezier::bezier exact; 0x558c80 bezier::set_points exact; 0x558cd0 bezier::set_points_array exact; 0x558d10 bezier::blend likely; 0x558d90 bezier::eval exact; 0x558e00 spline::spline exact; 0x558e20 spline::spline exact; 0x558e50 spline::set_points exact; 0x558f40 spline::eval likely
- unnamed_bitarray_0x55bd80.cpp: 0x55bd80 BitArray::BitArray exact; 0x55bd90 BitArray::~BitArray exact; 0x55bda0 BitArray::alloc exact; 0x55bde0 BitArray::attach exact; 0x55be00 BitArray::alloc_from_pool exact; 0x55be40 BitArray::fill exact; 0x55be80 BitArray::get exact; 0x55beb0 BitArray::set exact; 0x55bef0 BitArray::free exact; 0x55bf20 BitArray::all_equal likely
- unnamed_clip_0x5590b0.cpp: 0x5590b0 init_gr_clip_temp_verts likely; 0x5590c0 gr_clip_temp_verts likely; 0x5590e0 gr_clip_edge likely; 0x559330 gr_clip_alloc_temp_vertex exact; 0x559350 gr_clip_line exact; 0x5593a0 gr_clip_plane exact; 0x559490 gr_clip_polygon exact; 0x5594f0 gr_classify_sphere_view likely; 0x5595e0 bbox_from_points exact; 0x559660 bbox_from_segment exact; 0x5596b0 bbox_overlap exact; 0x559710 bbox_add_point exact; 0x559790 bbox_union exact; 0x559810 bbox_expand_by_vector exact; 0x559890 bbox_silhouette_corners exact; 0x559960 bbox_get_corner exact; 0x559a80 bbox_corner_indices_from_normal exact; 0x559b80 bbox_straddles_plane exact; 0x559c20 bbox_transform likely; 0x559da0 vector3::operator exact
- unnamed_ds3d_0x55ddb0.cpp: 0x55ddb0 ds3d_update_buffer exact; 0x55de30 ds3d_update_listener exact; 0x55def0 ds3d_close exact; 0x55df10 ds3d_release_listener exact; 0x55df30 ds3d_init likely; 0x55dfe0 ds3d_init_listener exact
- unnamed_ds_0x55bf90.cpp: 0x55bf90 init_ds_channels exact; 0x55bfa0 ds_channels exact; 0x55bfc0 ds_is_3d_buffer exact; 0x55c000 ds_duplicate_buffer exact; 0x55c080 ds_eax_init likely; 0x55c1a0 ds_init likely; 0x55c3f0 ds_init_buffers exact; 0x55c430 ds_init_channels exact; 0x55c450 ds_channel_init exact; 0x55c480 ds_init_volume_table exact; 0x55c4e0 ds_close exact; 0x55c590 ds_close_channels exact; 0x55c5b0 ds_channel_release likely; 0x55c6c0 ds_close_buffers exact; 0x55c6e0 ds_sound_release likely; 0x55c890 ds_load_buffer likely; 0x55c990 ds_load_buffer_data exact; 0x55ca10 ds_create_buffer likely; 0x55cae0 ds_fill_buffer likely; 0x55cb60 ds_get_3d_interface exact; 0x55cbe0 ds_load_buffer_acm likely; 0x55cd80 ds_wav_open exact; 0x55ce30 ds_find_free_buffer exact; 0x55ce60 ds_find_buffer exact; 0x55ced0 ds_unload_buffer exact; 0x55cf00 ds_play_3d likely; 0x55d070 ds_volume_to_attenuation exact; 0x55d0a0 ds_find_free_channel likely; 0x55d110 ds_play likely; 0x55d2b0 ds_play_streamed likely; 0x55d400 ds_stream_open likely; 0x55d530 ds_stream_read_chunk likely; 0x55d620 ds_stop exact; 0x55d640 ds_stop_fade exact; 0x55d6c0 ds_do_frame exact; 0x55d6f0 ds_channel_update_fade exact; 0x55d780 ds_channel_is_playing exact; 0x55d7b0 ds_pause exact; 0x55d800 ds_pause_all likely; 0x55d850 ds_unpause exact; 0x55d880 ds_channel_unpause exact; 0x55d8c0 ds_unpause_all exact; 0x55d8f0 ds_stop_all exact; 0x55d910 ds_set_volume exact; 0x55d960 ds_set_pan exact; 0x55d9a0 ds_set_frequency exact; 0x55da20 ds_is_playing exact; 0x55da50 ds_get_format exact; 0x55dac0 ds_get_frequency exact; 0x55db00 ds_get_channel exact; 0x55db30 ds_get_num_playing_channels exact; 0x55db50 ds_get_primary_buffer exact; 0x55db60 ds_get_direct_sound exact; 0x55db70 ds_get_eax_property_set exact; 0x55db80 ds_error_string likely; 0x55dc70 ds_is_ds3d_enabled exact; 0x55dc80 ds_is_eax_enabled exact; 0x55dc90 ds_disable_3d_eax exact; 0x55dca0 ds_disable_eax exact; 0x55dcb0 ds_set_group_volume exact; 0x55dd10 ds_estimate_duration likely; 0x55dd80 ds_set_eax_environment exact; 0x55dda0 ds_channel__ctor exact
- unnamed_ftp_0x559ea0.cpp: 0x559ea0 ftp_worker_thread_entry exact; 0x559eb0 ftp_get::abort_get exact; 0x559ed0 ftp_get::ftp_get likely; 0x55a150 ftp_get::~ftp_get exact; 0x55a1c0 ftp_get::get_status exact; 0x55a1d0 ftp_get::get_bytes_in exact; 0x55a1e0 ftp_get::get_total_bytes exact; 0x55a1f0 ftp_get::worker_thread exact; 0x55a220 ftp_get::get_file likely; 0x55a3f0 ftp_get::send_port likely; 0x55a4f0 ftp_get::connect_control_socket exact; 0x55a5b0 ftp_get::login_host exact; 0x55a650 ftp_get::send_command exact; 0x55a6a0 ftp_get::read_server_reply likely; 0x55a780 ftp_get::read_data_channel likely; 0x55a850 ftp_get::read_control_channel likely
- unnamed_http_0x55a8e0.cpp: 0x55a8e0 http_obj_thread exact; 0x55a900 http_get::abort_get exact; 0x55a940 http_get::http_get exact; 0x55a980 http_get::http_get exact; 0x55a9a0 http_get::init likely; 0x55ab70 http_get::~http_get exact; 0x55aba0 http_get::get_status exact; 0x55abb0 http_get::get_bytes_in exact; 0x55abc0 http_get::get_total_bytes exact; 0x55abd0 http_get::send_get likely; 0x55ae20 http_get::connect_socket likely; 0x55b0d0 http_get::get_line likely; 0x55b1c0 http_get::read_data_channel likely; 0x55b310 http_async_gethostbyname likely; 0x55b3e0 http_resolve_host exact
- unnamed_jpeg_write_0x5565e0.cpp: 0x5565e0 jpeg_write_file likely
- unnamed_lfsr_0x559db0.cpp: 0x559db0 lfsr_cipher::set_key exact; 0x559df0 lfsr_cipher::get_key exact; 0x559e10 lfsr_cipher::crypt likely
- unnamed_linefile_0x558340.cpp: 0x558340 LineFile::open_read exact; 0x5583d0 LineFile::open_write exact; 0x558430 LineFile::detect_eol likely; 0x5584f0 LineFile::fill_buffer exact; 0x558580 LineFile::LineFile exact; 0x558590 LineFile::~LineFile exact; 0x5585c0 LineFile::close exact; 0x558600 LineFile::read_line likely; 0x5587c0 LineFile::write likely; 0x5588d0 LineFile_printf exact; 0x558910 LineFile_open exact; 0x558960 LineFile_create exact; 0x5589b0 LineFile::putc exact; 0x558a10 LineFile::flush exact
- unnamed_poly_split_0x55f110.cpp: 0x55f110 gr_poly_split_sections_draw likely; 0x55f580 gr_poly_split_at_tex_line likely; 0x55f8b0 float_strictly_between exact; 0x55f900 gr_poly_split_edge_vertex likely; 0x55fb00 gr_poly_get_section likely
- unnamed_sndstream_0x566560.cpp: 0x566560 snd_start_timer exact; 0x566580 snd_timer_proc exact; 0x5665b0 snd_stream_kill_timer exact; 0x5665d0 snd_stream_get_writable exact; 0x566630 snd_stream_fill_acm likely; 0x566820 snd_stream_fill_pcm likely; 0x566970 snd_stream_fill_silence likely; 0x566ab0 snd_stream_stop exact; 0x566b10 snd_stream_rewind exact; 0x566b60 snd_stream_service likely; 0x566c30 snd_stream_find_free exact; 0x566c60 snd_stream_start exact; 0x566cf0 snd_stream_init exact; 0x566d30 snd_stream_close_all exact; 0x566d70 acm_driver_enum_callback likely; 0x566e20 acm_stream_open likely; 0x566eb0 acm_stream_close exact; 0x566ed0 acm_convert likely; 0x566f80 acm_stream_size exact; 0x566fb0 acm_stream_size_b exact; 0x566fe0 acm_init exact; 0x567080 acm_close exact; 0x5670a0 wav_open likely; 0x567300 wav_descend_data exact; 0x567350 wav_read exact; 0x567410 wav_close exact
- unnamed_string_0x5566e0.cpp: 0x5566e0 String::assign_n exact; 0x556760 String::assign_char exact; 0x556790 String::alloc exact; 0x5567d0 String::free exact; 0x5567f0 String::String exact; 0x556810 String::String exact; 0x556830 String::String exact; 0x556850 String::String exact; 0x556880 String::String exact; 0x5568b0 String::~String exact; 0x5568c0 String::c_str exact; 0x5568d0 String::size exact; 0x5568f0 String::empty exact; 0x556910 String::is_blank exact; 0x556950 String::to_lower likely; 0x556990 String::to_upper likely; 0x5569d0 String::substr exact; 0x556a80 String::left exact; 0x556b40 String::right exact; 0x556c00 String::mid exact; 0x556cb0 String::find_nocase likely; 0x556d50 String::trim_left exact; 0x556db0 String::trim_right exact; 0x556df0 String::to_float exact; 0x556e10 String::trim exact; 0x556e30 String::clear_thunk exact; 0x556e40 operator exact; 0x556ea0 operator exact; 0x556f30 operator exact; 0x556f70 (see file) exact; 0x557070 (see file) exact; 0x557180 (see file) exact; 0x557290 (see file) exact; 0x557360 (see file) exact; 0x557420 (see file) exact; 0x557490 (see file) exact; 0x557500 String::append_char exact; 0x557570 operator exact; 0x5575b0 operator exact; 0x5575f0 operator exact; 0x557630 (see file) exact; 0x557670 (see file) exact; 0x5576b0 (see file) exact; 0x5576f0 (see file) exact; 0x557740 (see file) exact; 0x557790 (see file) exact; 0x5577e0 (see file) exact; 0x557830 (see file) exact; 0x557880 (see file) exact; 0x5578d0 (see file) exact; 0x557920 (see file) exact; 0x557970 (see file) exact; 0x5579c0 (see file) exact; 0x557a10 (see file) exact; 0x557a60 (see file) exact; 0x557ab0 operator exact; 0x557ae0 String::from_int exact; 0x557b70 String::from_hex exact; 0x557c00 String::from_float exact; 0x557c90 String::format_bytes likely; 0x557e10 String_from_vector3 likely; 0x558080 String::from_hex_b exact; 0x558110 TextBuffer::TextBuffer exact; 0x558180 TextBuffer::~TextBuffer exact; 0x5581a0 TextBuffer::clear exact; 0x5581c0 TextBuffer::get_text exact; 0x5581d0 TextBuffer::get_length exact; 0x5581e0 TextBuffer::append likely; 0x558300 TextBuffer_printf exact
- unnamed_ui_slider_0x55e040.cpp: 0x55e040 ui_draw_bevel likely; 0x55e1a0 ui_draw_raised exact; 0x55e210 ui_draw_sunken exact; 0x55e280 ui_gadget::fill_inset exact; 0x55e2e0 ui_gadget::draw_bevel exact; 0x55e320 ui_gadget::draw_raised exact; 0x55e3a0 ui_gadget::draw_sunken exact; 0x55e420 ui_slider_base::get_marker_rect exact; 0x55e4c0 ui_slider_base::process likely; 0x55e780 ui_slider_base::ui_slider_base exact; 0x55e7e0 ui_slider_base::create likely; 0x55ea50 ui_slider_base::set_tooltip likely; 0x55eb10 ui_slider_base::get_pos exact; 0x55eb20 ui_slider_base::get_count exact; 0x55eb30 ui_slider_base::set_pos exact; 0x55eb40 ui_slider_base::set_count exact; 0x55eb50 ui_slider_base::set_marker_size exact; 0x55eb60 ui_slider_base::set_page_size exact; 0x55eb70 ui_slider_base::draw likely; 0x55ed30 ui_slider::create_scrollbar likely; 0x55ee50 ui_slider::get_range exact; 0x55ee70 ui_slider::set_visible_count exact; 0x55eeb0 ui_slider::set_range exact; 0x55eed0 ui_slider::update_marker exact; 0x55ef50 ui_tooltip::~ui_tooltip exact; 0x55ef60 ui_tooltip::draw likely
- unnamed_vfx_channel_0x55b440.cpp: 0x55b440 fx_rec4c::clear likely; 0x55b460 fx_rec4c::update likely; 0x55b730 fx_rec4c::get_transform likely; 0x55b960 vfx_force_desc::relocate exact; 0x55b9c0 vfx_node::get_pos exact; 0x55b9e0 vfx_vec_channel::eval exact; 0x55ba30 vfx_node::get_orient exact; 0x55ba50 vfx_quat_channel::eval exact; 0x55baa0 vfx_force_desc::get_channel64 exact; 0x55bad0 vfx_force_desc::get_channel70 exact; 0x55baf0 vfx_force_desc::get_channel7c exact; 0x55bb10 vfx_force_desc::get_channel88 exact; 0x55bb30 vfx_force_desc::get_channel94 exact; 0x55bb50 vfx_vec_channel::relocate exact; 0x55bb80 vfx_quat_channel::relocate exact; 0x55bbb0 vfx_channel::clear likely; 0x55bbc0 vfx_channel::vfx_channel exact; 0x55bbd0 vfx_node_header::relocate exact; 0x55bc00 vfx_channel::eval likely
- unnamed_vfx_dummy_0x55fc70.cpp: 0x55fc70 fx_rec2c::reset likely; 0x55fc90 fx_rec2c::update likely; 0x55fed0 vfx_dummy_desc::relocate exact
- unnamed_vfx_emitter_0x55ff00.cpp: 0x55ff00 init_gr_mode_2f91a5c exact; 0x508160 gr_mode_vfx_flat_alpha exact; 0x562550 vfx_particle_get_bitmaps likely; 0x562600 vfx_particle_get_color likely; 0x5626b0 vfx_particle_get_fade likely; 0x562840 vfx_particle_get_billboard_axes likely; 0x55ff30 fx_recb0::spawn likely; 0x5601f0 fx_recb0::init_particle likely; 0x5605f0 vfx_particle_apply_forces likely; 0x560a00 fx_recb0::move_particles likely; 0x561190 vfx_emitter_desc::relocate exact; 0x561220 fx_recb0::init likely; 0x5612b0 fx_recb0::get_transform likely; 0x561670 fx_recb0::update likely; 0x561900 fx_recb0::render_d3d unsure; 0x561ec0 fx_recb0::render likely; 0x5622b0 vfx_emitter__get_rate exact; 0x562380 (see file) exact; 0x5622f0 (see file) exact; 0x5622d0 fixed12_to_float exact; 0x5623c0 vector3::operator exact; 0x562440 vfx_vec_pair__ctor exact
- unnamed_vfx_material_0x564d70.cpp: 0x564d70 vfx_tex_layer::get_frame_bitmap exact; 0x564d90 vfx_tex_layer::get_frame_bitmap likely; 0x564e40 vfx_tex_layer::get_frame_bitmap_frac likely; 0x564ef0 (see file) unsure; 0x564f10 vfx_stub_564f10 unsure; 0x564f20 vfx_material::vfx_material likely; 0x564f80 vfx_material::reset likely; 0x564ff0 vfx_material::eval_curve34 likely; 0x5650e0 vfx_material::eval_curve44 likely; 0x5651b0 vfx_material::eval_curve48 likely; 0x565280 vfx_material::relocate exact; 0x5652e0 vfx_material::load_bitmaps likely; 0x565360 vfx_tex_layer::relocate exact; 0x565380 vfx_tex_layer::load_bitmap likely; 0x5653e0 (see file) unsure; 0x565400 vfx_group__relocate exact; 0x565450 vfx_group_entry__relocate exact; 0x565470 vfx_camera_desc::get_transform likely; 0x5655c0 vfx_camera_desc::relocate exact; 0x5655f0 vfx_camera__get_pos exact
