# decomp69 notes

Ranges: 0x544e50-0x5470ff (70 functions), 0x53e520-0x5407a0 (22), 0x54bb30-0x54d5a0 (22), 0x5368a0-0x536940
(10). Every function in sum.symbols inside them has a `// 0xADDR  Confidence:` marker now, plus one hidden
function without a symbol (0x544f80). Headers are in `include/` (a copy of snapshot 9147a03 with the
changes listed in CHANGED-HEADERS.txt); decisions D5000-D5050 are at the end of `include/DECISIONS.md`.
All new files and all 204 snapshot .cpp files pass `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w`
against this include tree with 0 errors.

Counts below are exact / likely / unsure per file. Compiler-generated initialisers, implicit
constructors and header-inline COMDATs are counted when they have a marker.

## Units

| File | What | Range | Markers | e / l / u |
|---|---|---|---|---|
| `vsdk/unnamed_gr_line_0x53e520.cpp` | 3D drawing helpers: gr_line_3d and its back-end dispatch, gr_line_3d_world, the `use_buffered_alpha_polys` console command, gr_poly, two clip-and-project helpers, camera-facing and oriented bitmaps, gr_line_quad_3d, the debug sphere / box / arrows / axes; the gr::Vertex assignment COMDAT | 0x53e520-0x54080f | 22 | 4 / 18 / 0 |
| `vsdk/unnamed_vfile_array_0x544e50.cpp` | File array read / write / sync helpers (vector3, float pair, int, int16) | 0x544e50-0x5450bb | 9 | 7 / 2 / 0 |
| `vsdk/unnamed_gr_poly_0x5450c0.cpp` | the polygon builder: begin / vertex / set_texture / uv / rgb / alpha / end, the multipass clipper, the sorted command buffer and its replay (gr_poly_buffer_flush) | 0x5450c0-0x54614a | 29 | 19 / 10 / 0 |
| `vsdk/unnamed_sound_emitter_0x546150.cpp` | sound emitters (128 records, linked list, voice stealing, repeat delays, segment sources) | 0x546150-0x546cf0 | 27 | 21 / 6 / 0 |
| `vsdk/unnamed_perlin_0x546d00.cpp` | Ken Perlin's reference noise (float version) | 0x546d00-0x54738e | 6 | 3 / 3 / 0 |
| `vsdk/unnamed_d3d_tcache_0x54bb30.cpp` | the Direct3D texture cache (sections, managed surfaces, mip levels, pixel format conversion, the two stage records) | 0x54bb30-0x54d5b5 | 22 | 16 / 6 / 0 |
| `vsdk/gr_direct3d_additions_0x5368a0.cpp` | markers only, to fold into `vsdk/gr_direct3d.cpp` PART 1: the gr::Mode accessor COMDATs; their bodies are gr.h's inline members (checked, unchanged) | 0x5368a0-0x536930 | 10 | 10 / 0 / 0 |

Total 125 markers (124 sum.symbols functions + 0x544f80): 80 exact, 45 likely, 0 unsure.

Not new code, only markers: 0x5450c0 / 0x5450f0 / 0x546150 / 0x546180 / 0x53e670 / 0x54bb30 (initialisers),
0x546110 and 0x546c80 (implicit constructors), 0x540790 (implicit gr::Vertex operator=), 0x546cc0
(vector3::operator!=, now in vector.h), 0x546120 / 0x546130 (inline in the new poly header), 0x54d470-0x54d5a0
(the stage record inlines, gr.h / gr_direct3d.h), 0x5368a0-0x536930 (gr.h).

Unit boundaries: D5000 / D5001. Each object starts with its own initialisers or ends with its own COMDATs,
and its .data / .bss block lies between its neighbours' in code order. No path strings and no strings.tbl
blocks, so all names are placeholders. modules.txt is empty: nothing proves a file name.

## Game bugs

1. **gr_poly_rgb tint (0x545610), live, sure about the code.** With gr_set_tint on, the red channel gets
   the green tint's result, green stays untinted and the red tint's result is thrown away (D5012).
   gr_poly_set_texture (0x545340) does it right. The tint is on while levelrender_draw_list draws hovered
   characters and tinted effect objects; only polygons drawn through gr_poly with per-vertex colours
   (vertex flag 4) are affected, so the visible effect is probably small and rare (not checked in game).
2. **Direct3D textures of multi-section bitmaps (0x54c0a0), latent on modern cards, fairly sure.** For every
   format but 8-bit alpha, gr_d3d_copy_bitmap_to_surface copies bm_w x bm_h texels (the whole bitmap's size,
   which gr_d3d_create_texture_section passes) starting at the section's corner into a tex_w x tex_h surface.
   With one section bm_w <= tex_w and it is fine; when a bitmap is wider or taller than the device's maximum
   texture size (gr_screen.max_bitmap_w/h = dwMaxTextureWidth/Height, 256 on Voodoo-class cards) each row
   overruns into the next and the last rows overrun the locked surface, and the reads run past the bitmap.
   On cards with 2048+ textures every game bitmap is one section. The Glide back end loops over the
   section's w x h (gr_glide_create_texture_section), which looks like the intent.
3. **8-bit alpha textures (0x54c0a0), latent, sure about the code.** The 8-bit alpha branch ignores the
   section position (like Glide's, D2505) and reads tex_w bytes from each bitmap row with stride bm_w: for a
   bitmap narrower or shorter than its power-of-two texture it reads past the end of the bitmap data (by up
   to (tex_h - bm_h) * bm_w + tex_w - bm_w bytes). The extra texels are outside the u / v scale, so nothing
   wrong is drawn; only a read past the allocation.
4. **Mip level Lock (0x54bf46), latent.** IDirectDrawSurface7::Lock is called with flags 0 (no
   DDLOCK_WAIT); if it fails, the function releases the level but not the AddRef'd top surface and leaves the
   bitmap locked (bm_unlock is only on the CreateSurface failure path), and the section keeps a NULL surface.
5. **gr_clip_and_project_poly (0x53e9d0), latent.** A vertex behind the viewer after clipping returns 0
   without gr_clip_free_temp_verts; gr_clip_alloc_temp_vertex has no bound check (64 vertices), but
   gr_setup_3d resets the count every frame. Only levelrender_face_vertex_colors calls it.
6. **Precedence slip `!pnt->flags & 1` (0x53eb37, 0x53ee27), no effect.** gr_bitmap_3d_projected and
   gr_bitmap_3d_oriented_quad mean "return when not projected"; the test is never true after
   gr_project_vertex, and both callers test the projected bit first (D5005).
7. Dead code only: gr_poly_project_to_screen (0x53e8d0, no caller) returns the vertex count for a polygon
   that is still outside one plane after clipping, without writing the output; sound emitter flag 8 is
   cleared in three places and set nowhere (D5032); the DirectSound3D paths of sound_emitter_play /
   update_position are behind sound_emitter_stub_false, which always returns false.

## Disagreements with sum.symbols / docs

- 0x5466f0 sound_create_emitter: the second argument is the position update interval in ms (+0x48), not a
  volume (snd.h said "volume 100"; D5030).
- 0x546840 sound_emitter_set_pos_vel: (idx, seg_a, seg_b, vel, scale), it sets the segment ends and calls the
  position update with seg_a (D5034).
- 0x53faf0 gr_draw_arrow_3d draws a wire box (pos, orient, size); 0x53edd0 gr_bitmap_3d_oriented_quad uses
  no bm_get_dimensions, gr_rotate_vertex or gr_check_normal_facing (its sum.symbols comment describes
  0x53f060); 0x5400a0's head sits at the middle of the shaft.
- 0x54d470 d3d_tcache_slot__ctor / 0x54bb30 init_d3d_tcache_slots_2f132d8: the constructor and initialiser of the
  Direct3D cache's two stage records (d3d_tcache_slot), not an OpenGL record (D5021).
- 0x54d330 gr_d3d_is_texture_size returns whether both sizes already are texture sizes (4 .. 128, 256 for
  anything larger); it does not return a rounded size.
- 0x546110 gr_mode_slot__ctor is the implicit constructor of gr_tmapper_pass (the slots are pass records).
- 0x2f075b8 gr_poly_multitexture means "a pass has a sectioned bitmap" (it sends the polygon to
  gr_poly_split_sections_draw), not multitexturing; name kept.
- 0x5aca38 / 0x5aca3c / 0x5aca44 (no symbols): the buffer limit (end - 0x200), the buffer's last byte (no
  reference) and a first-overflow flag; 0x2f076bc / 0x2f076e4 (no symbols): the fixed-depth flag and
  gr_poly_begin's unused third argument.
- source-map.md lists 0x54bb30-0x54d5a0 as one row; the unit really ends at 0x54d5b5 (invalidate_bitmap's
  `ret 4`) and gr_opengl.cpp starts at 0x54d5c0 (consistent).

## Signature changes (callers in the snapshot compile unchanged)

gr_poly, gr_poly_begin, gr_poly_set_texture, gr_poly_end, gr_bitmap_3d, gr_draw_sphere, gr_draw_arrow_3d,
gr_line_quad_3d (gr.h); gr_d3d_tcache_set (gr_direct3d.h); sound_emitters_init, sound_create_emitter
(parameter name only), sound_emitter_delete (snd.h). See CHANGED-HEADERS.txt.

## Left open

- D1605 (gr_vertex constructor / vector3 member): the functions that construct or assign vertices in the
  binary are written without it and marked likely (D5002).
- The remaining uncertainty is VC6's ordering of
  or'ed terms in gr_d3d_copy_bitmap_to_surface, the shape of the zero-fill loops (VC6 turns them into rep
  stos; gr_glide.cpp makes the same choice) and the evaluation order of the s-curve products in the noise.
- Nothing in the ranges was left unwritten.
