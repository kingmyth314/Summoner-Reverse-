// new_options.cpp - Sum.exe v1.40, code 0x41ee90-0x4205ae (screen 12, the options screen)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .rdata 0x580134-0x5801ff (layout tables; the floats 0x580200-0x58021f are __real@ constants of
// this unit's code), .data 0x58e0d0-0x58f38f (button table), .bss 0x5c32d8-0x5c3a2f.
// options_draw_version (0x4146c0) is not in this unit: sum.symbols already moved it to the interface code.

#include <string.h>
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/key.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/snd/snd.h"
#include "engine/rendering/levelrender.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_options.h"
#include "experimental/ui_shift.h"   // EXPERIMENTAL_BUILD only (widescreen); keep it the last include

#define OPTIONS_NUM_BUTTONS 75

// ---- .rdata --------------------------------------------------------------------------------
// Per resolution_index (640, 800, 1024). Names (*new*).
const int options_slider_hit_margin[3] = { 8, 10, 12 };                                     // 0x580134
const int options_sfx_slider_pos[3][2]     = { { 59, 99 },   { 74, 124 },  { 94, 155 } };    // 0x580140 x, y
const int options_sfx_slider_size[3][2]    = { { 117, 8 },   { 146, 10 },  { 187, 20 } };    // 0x580158 w, h
const int options_music_slider_pos[3][2]   = { { 59, 146 },  { 74, 183 },  { 94, 230 } };    // 0x580170
const int options_music_slider_size[3][2]  = { { 117, 8 },   { 146, 10 },  { 187, 20 } };    // 0x580188
const int options_camera_slider_pos[3][2]  = { { 210, 334 }, { 263, 418 }, { 323, 537 } };   // 0x5801a0
const int options_camera_slider_size[3][2] = { { 90, 8 },    { 113, 10 },  { 161, 20 } };    // 0x5801b8
const int options_gamma_slider_pos[3][2]   = { { 449, 435 }, { 563, 546 }, { 726, 703 } };   // 0x5801d0
const int options_gamma_slider_size[3][2]  = { { 143, 8 },   { 177, 10 },  { 238, 20 } };    // 0x5801e8

// ---- .data ---------------------------------------------------------------------------------
// 0x58e0d0  75 x 0x40 (sum.symbols options_button_defs = 0x58e10c, the flags field of entry 0).
// flags: 1 HiLite (acts through the Press entry after it, on release), 2 check box hot spot that acts
// itself, 4 slider knob (dragged, hit area widened by options_slider_hit_margin). The "checkbox" entry
// before each "checkbox-hilite" is the tick, shown by bit 0 of its state.
ui_button_def options_button_defs[OPTIONS_NUM_BUTTONS] = {
    { "Options-Bkgrnd",     { { 0, 0 },     { 0, 0 },     { 0, 0 } },     0 },
    { "Meter-Drag-Bar",     { { 59, 90 },   { 74, 113 },  { 94, 145 } },  4 },   //  1 effects volume knob
    { "Meter-Drag-Bar",     { { 59, 137 },  { 74, 171 },  { 94, 220 } },  4 },   //  2 music volume knob
    { "checkbox",           { { 98, 208 },  { 123, 260 }, { 155, 342 } }, 0 },   //  3 EAX on
    { "checkbox-hilite",    { { 98, 208 },  { 123, 260 }, { 155, 342 } }, 2 },
    { "checkbox",           { { 160, 208 }, { 200, 260 }, { 252, 342 } }, 0 },   //  5 EAX off
    { "checkbox-hilite",    { { 160, 208 }, { 200, 260 }, { 252, 342 } }, 2 },
    { "checkbox",           { { 83, 287 },  { 104, 358 }, { 155, 474 } }, 0 },   //  7 solo confirm on
    { "checkbox-hilite",    { { 83, 287 },  { 104, 358 }, { 155, 474 } }, 2 },
    { "checkbox",           { { 145, 287 }, { 181, 358 }, { 252, 474 } }, 0 },   //  9 solo confirm off
    { "checkbox-hilite",    { { 145, 287 }, { 181, 358 }, { 252, 474 } }, 2 },
    { "checkbox",           { { 222, 286 }, { 278, 358 }, { 381, 474 } }, 0 },   // 11 combat flag on
    { "checkbox-hilite",    { { 222, 286 }, { 278, 358 }, { 381, 474 } }, 2 },
    { "checkbox",           { { 307, 286 }, { 384, 358 }, { 509, 474 } }, 0 },   // 13 combat flag off
    { "checkbox-hilite",    { { 307, 286 }, { 384, 358 }, { 509, 474 } }, 2 },
    { "checkbox",           { { 88, 331 },  { 110, 414 }, { 149, 534 } }, 0 },   // 15 invert camera on
    { "checkbox-hilite",    { { 88, 331 },  { 110, 414 }, { 149, 534 } }, 2 },
    { "checkbox",           { { 150, 331 }, { 188, 414 }, { 246, 534 } }, 0 },   // 17 invert camera off
    { "checkbox-hilite",    { { 150, 331 }, { 188, 414 }, { 246, 534 } }, 2 },
    { "Meter-Drag-Bar",     { { 210, 325 }, { 263, 406 }, { 328, 527 } }, 4 },   // 19 camera speed knob
    { "checkbox",           { { 169, 376 }, { 211, 470 }, { 286, 611 } }, 0 },   // 20 dialogue camera 0
    { "checkbox-hilite",    { { 169, 376 }, { 211, 470 }, { 286, 611 } }, 2 },
    { "checkbox",           { { 259, 376 }, { 324, 470 }, { 423, 611 } }, 0 },   // 22 dialogue camera 1
    { "checkbox-hilite",    { { 259, 376 }, { 324, 470 }, { 423, 611 } }, 2 },
    { "checkbox",           { { 259, 396 }, { 324, 495 }, { 423, 640 } }, 0 },   // 24 dialogue camera 2
    { "checkbox-hilite",    { { 259, 396 }, { 324, 495 }, { 423, 640 } }, 2 },
    { "checkbox",           { { 145, 438 }, { 181, 548 }, { 240, 702 } }, 0 },   // 26 cfg +0x3c off
    { "checkbox-hilite",    { { 145, 438 }, { 181, 548 }, { 240, 702 } }, 2 },
    { "checkbox",           { { 264, 438 }, { 330, 548 }, { 415, 702 } }, 0 },   // 28 cfg +0x3c on
    { "checkbox-hilite",    { { 264, 438 }, { 330, 548 }, { 415, 702 } }, 2 },
    { "checkbox",           { { 429, 122 }, { 536, 154 }, { 688, 218 } }, 0 },   // 30 party model detail 0
    { "checkbox-hilite",    { { 429, 122 }, { 536, 154 }, { 688, 218 } }, 2 },
    { "checkbox",           { { 429, 141 }, { 536, 177 }, { 688, 247 } }, 0 },   // 32 party model detail 1
    { "checkbox-hilite",    { { 429, 141 }, { 536, 177 }, { 688, 247 } }, 2 },
    { "checkbox",           { { 429, 160 }, { 536, 201 }, { 688, 276 } }, 0 },   // 34 party model detail 2
    { "checkbox-hilite",    { { 429, 160 }, { 536, 201 }, { 688, 276 } }, 2 },
    { "checkbox",           { { 575, 122 }, { 720, 154 }, { 920, 218 } }, 0 },   // 36 model detail 0
    { "checkbox-hilite",    { { 575, 122 }, { 720, 154 }, { 920, 218 } }, 2 },
    { "checkbox",           { { 575, 141 }, { 720, 176 }, { 920, 247 } }, 0 },   // 38 model detail 1
    { "checkbox-hilite",    { { 575, 141 }, { 720, 176 }, { 920, 247 } }, 2 },
    { "checkbox",           { { 575, 160 }, { 720, 200 }, { 920, 276 } }, 0 },   // 40 model detail 2
    { "checkbox-hilite",    { { 575, 160 }, { 720, 200 }, { 920, 276 } }, 2 },
    { "checkbox",           { { 443, 204 }, { 554, 255 }, { 718, 346 } }, 0 },   // 42 texture quality 0 / 1
    { "checkbox-hilite",    { { 443, 204 }, { 554, 255 }, { 718, 346 } }, 2 },
    { "checkbox",           { { 543, 204 }, { 679, 255 }, { 877, 346 } }, 0 },   // 44 texture quality 2
    { "checkbox-hilite",    { { 543, 204 }, { 679, 255 }, { 877, 346 } }, 2 },
    { "checkbox",           { { 555, 243 }, { 694, 304 }, { 887, 429 } }, 0 },   // 46 lightmaps off (toggle)
    { "checkbox-hilite",    { { 555, 243 }, { 694, 304 }, { 887, 429 } }, 2 },
    { "checkbox",           { { 555, 262 }, { 694, 328 }, { 887, 457 } }, 0 },   // 48 cfg +0x21 (toggle)
    { "checkbox-hilite",    { { 555, 262 }, { 694, 328 }, { 887, 457 } }, 2 },
    { "checkbox",           { { 417, 307 }, { 521, 384 }, { 668, 531 } }, 0 },   // 50 detail +0x24 0
    { "checkbox-hilite",    { { 417, 307 }, { 521, 384 }, { 668, 531 } }, 2 },
    { "checkbox",           { { 497, 307 }, { 621, 384 }, { 794, 531 } }, 0 },   // 52 detail +0x24 1
    { "checkbox-hilite",    { { 497, 307 }, { 621, 384 }, { 794, 531 } }, 2 },
    { "checkbox",           { { 580, 307 }, { 725, 384 }, { 927, 531 } }, 0 },   // 54 detail +0x24 2
    { "checkbox-hilite",    { { 580, 307 }, { 725, 384 }, { 927, 531 } }, 2 },
    { "checkbox",           { { 417, 352 }, { 522, 440 }, { 668, 599 } }, 0 },   // 56 fog 0
    { "checkbox-hilite",    { { 417, 352 }, { 522, 440 }, { 668, 599 } }, 2 },
    { "checkbox",           { { 497, 352 }, { 622, 440 }, { 794, 599 } }, 0 },   // 58 fog 1
    { "checkbox-hilite",    { { 497, 352 }, { 622, 440 }, { 794, 599 } }, 2 },
    { "checkbox",           { { 580, 352 }, { 725, 440 }, { 927, 599 } }, 0 },   // 60 fog 2
    { "checkbox-hilite",    { { 580, 352 }, { 725, 440 }, { 927, 599 } }, 2 },
    { "checkbox",           { { 398, 398 }, { 498, 498 }, { 644, 661 } }, 0 },   // 62 antialias on (hidden
    { "checkbox-hilite",    { { 398, 398 }, { 498, 498 }, { 644, 661 } }, 2 },   //    without card support)
    { "checkbox",           { { 462, 398 }, { 578, 498 }, { 741, 661 } }, 0 },   // 64 antialias off
    { "checkbox-hilite",    { { 462, 398 }, { 578, 498 }, { 741, 661 } }, 2 },
    { "checkbox",           { { 530, 398 }, { 663, 498 }, { 859, 661 } }, 0 },   // 66 cfg +0x34 on
    { "checkbox-hilite",    { { 530, 398 }, { 663, 498 }, { 859, 661 } }, 2 },
    { "checkbox",           { { 592, 398 }, { 740, 498 }, { 956, 661 } }, 0 },   // 68 cfg +0x34 off
    { "checkbox-hilite",    { { 592, 398 }, { 740, 498 }, { 956, 661 } }, 2 },
    { "Meter-Drag-Bar",     { { 445, 426 }, { 532, 534 }, { 712, 693 } }, 4 },   // 70 gamma knob
    { "Icon-CheckHilite",   { { 551, 18 },  { 689, 23 },  { 882, 31 } },  1 },   // 71 accept
    { "Icon-CheckPress",    { { 551, 18 },  { 689, 23 },  { 882, 31 } },  0 },
    { "Icon-GrnClseHilite", { { 599, 18 },  { 749, 23 },  { 959, 32 } },  1 },   // 73 cancel
    { "Icon-GrnClsePress",  { { 599, 18 },  { 749, 23 },  { 959, 32 } },  0 },
};

// ---- .bss ----------------------------------------------------------------------------------
int options_mouse_mode;                                   // 0x5c32d8  0 idle, 1 button held, 2 dragging a knob
unsigned int options_flags;                                  // 0x5c32dc  1 paused the game, 2 opened from the
                                                          //           main menu, 4 accepted (save on leave)
SummonerOptions options_edit;                             // 0x5c32e0  working copy of Summoner_options
int options_pressed_button;                               // 0x5c3320
// 0x5c3324: 4 bytes nothing reads or writes
ui_button_state options_buttons[OPTIONS_NUM_BUTTONS];     // 0x5c3328

// init_options_config_defaults_thunk (0x41ee90, "jmp 0x41eea0") and init_options_config_defaults (0x41eea0)
// are the compiler's dynamic initialiser pair for options_edit: the inline SummonerOptions constructor
// (static_initializers entry 61; no destructor, so no atexit). Confidence: exact (no source of their own).
// 0x41ee90 init_options_config_defaults_thunk (jmp, entry 61). Confidence: exact.
// 0x41eea0 init_options_config_defaults (the inline SummonerOptions constructor on options_edit). Confidence: exact.

// 0x41ef30  Confidence: exact (VC6 byte match, D23602) (byte table 0x41f09c over i 0..70 into the two-entry
// jump table 0x41f094: the entries that are always drawn are written as a switch inside the load branch).
void options_load_bitmaps()
{
    float scale = ui_get_scale();
    int i;

    for (i = 0; i < OPTIONS_NUM_BUTTONS; i++) {
        ui_button_def *d = &options_button_defs[i];   // D23602: d declared before b (i reloaded into EDI)
        ui_button_state *b = &options_buttons[i];
        b->flags = 0;
        b->bitmap = -1;
        b->x = 0;
        b->y = 0;
        b->w = 0;
        b->h = 0;

        if (strlen(d->name)) {
            char filename[128];

            strcpy(filename, d->name);
            strcat(filename, ui_bitmap_suffix_tga[ui_hires]);
            b->bitmap = bm_load(filename, path_interface, -1);
            b->x = d->pos[resolution_index][0];
            b->y = d->pos[resolution_index][1];
            bm_get_dimensions(b->bitmap, &b->w, &b->h);
            b->w = (int)(b->w * scale + 0.5f);
            b->h = (int)(b->h * scale + 0.5f);
            switch (i) {
            case 0:
            case 1:
            case 2:
            case 19:
            case 70:
                b->flags |= 1;
                break;
            }
        }
    }
}

// 0x41f0f0  Confidence: likely (options_mouse_mode is stored next to the rep movsd of the copy).
// in_game = the previous screen was not the main menu (interface_set_screen passes prev != 14).
void options_init(bool in_game)
{
    options_flags = 0;
    if (in_game) {
        if (!game_is_paused() && (game_flags & 1)) {
            options_flags |= 1;
            game_pause(0, 0);
        }
    } else {
        options_flags = 2;
    }

    options_mouse_mode = 0;
    options_edit = Summoner_options;

    options_set_sfx_volume(options_edit.sfx_volume);
    options_set_music_volume(options_edit.music_volume);
    options_set_eax(options_edit.eax);
    options_set_solo_confirm(options_edit.solo_confirm);
    options_set_invert_camera(options_edit.invert_camera);
    options_set_camera_speed(options_edit.camera_speed);
    options_set_dialogue_camera(options_edit.dialogue_camera);
    options_set_cfg_3c(options_edit.cfg_3c);
    options_set_combat_flag(options_edit.combat_flag);
    options_set_party_model_detail(options_edit.detail_14);
    options_set_model_detail(options_edit.detail_18);
    options_set_texture_quality(options_edit.texture_quality);
    options_set_lightmaps_off(options_edit.lightmaps_off);
    options_set_cfg_21(options_edit.cfg_21);
    options_set_detail_24(options_edit.detail_24);
    options_set_fog_quality(options_edit.fog_quality);
    options_set_antialias(options_edit.antialias);
    options_set_cfg_34(options_edit.cfg_34);
    options_apply_gamma(options_edit.gamma_correction);
}

// 0x41f250  Confidence: exact. The knob (entry 1) is centred on the slider position of the value.
void options_set_sfx_volume(float volume)
{
    options_edit.sfx_volume = volume;
    snd_set_group_volume(0, volume);
    options_buttons[1].x = options_sfx_slider_pos[resolution_index][0]
                         + (int)(options_sfx_slider_size[resolution_index][0] * volume + 0.5f)
                         - options_buttons[1].w / 2;
}

// 0x41f2b0  Confidence: exact.
void options_set_music_volume(float volume)
{
    options_edit.music_volume = volume;
    music_set_volume(volume);
    options_buttons[2].x = options_music_slider_pos[resolution_index][0]
                         + (int)(options_music_slider_size[resolution_index][0] * volume + 0.5f)
                         - options_buttons[2].w / 2;
}

// 0x41f300  Confidence: exact (VC6 byte match, match24; was likely) (the clamp tests: fcom + test ah, 0x41 is "<=", test ah, 1 / jne is ">=";
// the clamped value stays on the FPU stack for the knob). Range 1.0..2.0.
void options_set_camera_speed(float speed)
{
    if (speed <= 1.0f)
        speed = 1.0f;
    else if (speed >= 2.0f)
        speed = 2.0f;
    options_edit.camera_speed = speed;
    options_buttons[19].x = options_camera_slider_pos[resolution_index][0]
                          + (int)(options_camera_slider_size[resolution_index][0] * (speed - 1.0f) + 0.5f)
                          - options_buttons[19].w / 2;
}

// 0x41f380  Confidence: exact (VC6 byte match, match9; was likely: the binary multiplies by the float 2/3 (0x580204):
// written as the division
// by the 1.5 range, which VC6 is assumed to turn into the reciprocal; see NOTES). Range 0.7..2.2.
void options_apply_gamma(float gamma)
{
    if (gamma <= 0.7f)
        gamma = 0.7f;
    else if (gamma >= 2.2f)
        gamma = 2.2f;
    options_edit.gamma_correction = gamma;
    gr_set_gamma(gamma);
    float t = (gamma - 0.7f) / 1.5f;
    options_buttons[70].x = options_gamma_slider_pos[resolution_index][0]
                          + (int)(t * options_gamma_slider_size[resolution_index][0] + 0.5f)
                          - options_buttons[70].w / 2;
}

// 0x41f410  Confidence: exact (VC6 byte match, match24; was likely) (order of the two buttons in the no-EAX branch). Without EAX both radio
// buttons are unticked and disabled; nothing here enables them again.
void options_set_eax(bool on)
{
    options_edit.eax = on;
    if (snd_is_eax_enabled()) {
        if (on) {
            options_buttons[3].flags |= 1;
            options_buttons[5].flags &= ~1;
            eax_restore_reverb();
        } else {
            options_buttons[5].flags |= 1;
            options_buttons[3].flags &= ~1;
            eax_mute_reverb();
        }
    } else {
        options_buttons[5].flags = (options_buttons[5].flags & ~1) | 4;
        options_buttons[3].flags = (options_buttons[3].flags & ~1) | 4;
    }
}

// 0x41f490  Confidence: exact.
void options_set_solo_confirm(bool on)
{
    options_edit.solo_confirm = on;
    if (on) {
        options_buttons[7].flags |= 1;
        options_buttons[9].flags &= ~1;
    } else {
        options_buttons[9].flags |= 1;
        options_buttons[7].flags &= ~1;
    }
}

// 0x41f4d0  Confidence: exact.
void options_set_invert_camera(bool on)
{
    options_edit.invert_camera = on;
    if (on) {
        options_buttons[15].flags |= 1;
        options_buttons[17].flags &= ~1;
    } else {
        options_buttons[17].flags |= 1;
        options_buttons[15].flags &= ~1;
    }
}

// 0x41f510  Confidence: exact (VC6 byte match, match24; was likely) (statement order inside the cases).
void options_set_dialogue_camera(int mode)
{
    options_edit.dialogue_camera = mode;
    switch (mode) {
    case 0:
        options_buttons[20].flags |= 1;
        options_buttons[22].flags &= ~1;
        options_buttons[24].flags &= ~1;
        break;
    case 1:
        options_buttons[22].flags |= 1;
        options_buttons[20].flags &= ~1;
        options_buttons[24].flags &= ~1;
        break;
    case 2:
        options_buttons[24].flags |= 1;
        options_buttons[20].flags &= ~1;
        options_buttons[22].flags &= ~1;
        break;
    }
}

// 0x41f5c0  Confidence: exact. "On" is the right-hand box (28).
void options_set_cfg_3c(bool on)
{
    options_edit.cfg_3c = on;
    if (on) {
        options_buttons[28].flags |= 1;
        options_buttons[26].flags &= ~1;
    } else {
        options_buttons[26].flags |= 1;
        options_buttons[28].flags &= ~1;
    }
}

// 0x41f600  Confidence: exact.
void options_set_combat_flag(bool on)
{
    options_edit.combat_flag = on;
    if (on) {
        options_buttons[11].flags |= 1;
        options_buttons[13].flags &= ~1;
    } else {
        options_buttons[13].flags |= 1;
        options_buttons[11].flags &= ~1;
    }
}

// 0x41f640  Confidence: exact (VC6 byte match, match24; was likely) (as options_set_dialogue_camera).
void options_set_party_model_detail(int level)
{
    options_edit.detail_14 = level;
    switch (level) {
    case 0:
        options_buttons[30].flags |= 1;
        options_buttons[32].flags &= ~1;
        options_buttons[34].flags &= ~1;
        break;
    case 1:
        options_buttons[32].flags |= 1;
        options_buttons[30].flags &= ~1;
        options_buttons[34].flags &= ~1;
        break;
    case 2:
        options_buttons[34].flags |= 1;
        options_buttons[30].flags &= ~1;
        options_buttons[32].flags &= ~1;
        break;
    }
}

// 0x41f6f0  Confidence: exact (VC6 byte match, match24; was likely) (as options_set_dialogue_camera).
void options_set_model_detail(int level)
{
    options_edit.detail_18 = level;
    switch (level) {
    case 0:
        options_buttons[36].flags |= 1;
        options_buttons[38].flags &= ~1;
        options_buttons[40].flags &= ~1;
        break;
    case 1:
        options_buttons[38].flags |= 1;
        options_buttons[36].flags &= ~1;
        options_buttons[40].flags &= ~1;
        break;
    case 2:
        options_buttons[40].flags |= 1;
        options_buttons[36].flags &= ~1;
        options_buttons[38].flags &= ~1;
        break;
    }
}

// 0x41f7a0  Confidence: exact. Only two boxes: 0 and 1 both tick the first one.
void options_set_texture_quality(int quality)
{
    options_edit.texture_quality = quality;
    switch (quality) {
    case 0:
        options_buttons[42].flags |= 1;
        options_buttons[44].flags &= ~1;
        break;
    case 1:
        options_buttons[42].flags |= 1;
        options_buttons[44].flags &= ~1;
        break;
    case 2:
        options_buttons[44].flags |= 1;
        options_buttons[42].flags &= ~1;
        break;
    }
}

// 0x41f7f0  Confidence: exact. Without lightmap multitexturing the option is forced on.
void options_set_lightmaps_off(bool off)
{
    if (gr_is_mode_supported(&gr_mode_lightmap_multitexture)) {
        options_edit.lightmaps_off = off;
        if (off)
            options_buttons[46].flags |= 1;
        else
            options_buttons[46].flags &= ~1;
    } else {
        options_edit.lightmaps_off = true;
        options_buttons[46].flags |= 1;
    }
}

// 0x41f840  Confidence: exact.
void options_set_cfg_21(bool on)
{
    options_edit.cfg_21 = on;
    if (on)
        options_buttons[48].flags |= 1;
    else
        options_buttons[48].flags &= ~1;
}

// 0x41f870  Confidence: exact (VC6 byte match, match24; was likely) (as options_set_dialogue_camera).
void options_set_detail_24(int level)
{
    options_edit.detail_24 = level;
    switch (level) {
    case 0:
        options_buttons[50].flags |= 1;
        options_buttons[52].flags &= ~1;
        options_buttons[54].flags &= ~1;
        break;
    case 1:
        options_buttons[52].flags |= 1;
        options_buttons[50].flags &= ~1;
        options_buttons[54].flags &= ~1;
        break;
    case 2:
        options_buttons[54].flags |= 1;
        options_buttons[50].flags &= ~1;
        options_buttons[52].flags &= ~1;
        break;
    }
}

// 0x41f920  Confidence: exact (VC6 byte match, match24; was likely) (as options_set_dialogue_camera).
void options_set_fog_quality(int quality)
{
    options_edit.fog_quality = quality;
    switch (quality) {
    case 0:
        options_buttons[56].flags |= 1;
        options_buttons[58].flags &= ~1;
        options_buttons[60].flags &= ~1;
        break;
    case 1:
        options_buttons[58].flags |= 1;
        options_buttons[56].flags &= ~1;
        options_buttons[60].flags &= ~1;
        break;
    case 2:
        options_buttons[60].flags |= 1;
        options_buttons[56].flags &= ~1;
        options_buttons[58].flags &= ~1;
        break;
    }
}

// 0x41f9d0  Confidence: exact (VC6 byte match, match24; was likely) (neg / sbb / and is VC6's form of "c ? x : 0").
void options_set_antialias(bool on)
{
    options_edit.antialias = gr_antialias_enabled ? on : false;
    if (options_edit.antialias) {
        options_buttons[62].flags |= 1;
        options_buttons[64].flags &= ~1;
    } else {
        options_buttons[64].flags |= 1;
        options_buttons[62].flags &= ~1;
    }
}

// 0x41fa20  Confidence: exact.
void options_set_cfg_34(bool on)
{
    options_edit.cfg_34 = on;
    if (on) {
        options_buttons[66].flags |= 1;
        options_buttons[68].flags &= ~1;
    } else {
        options_buttons[68].flags |= 1;
        options_buttons[66].flags &= ~1;
    }
}

// 0x41fa60  Confidence: exact (both calls are tail jumps).
void options_leave()
{
    if (options_flags & 1)
        game_unpause();
    if (options_flags & 4)
        options_apply_and_save();
    else
        options_restore_volumes();
}

// folded (0x4063a0, ui_return_true): the screen has no panel of its own, so its mouse_in_panel returns true;
// the linker folded the identical copies into one (identical-code folding). interface_mouse_over_panel
// calls each by name (D7903); name ours.
bool options_mouse_in_panel(int x, int y)
{
    return true;
}

// 0x41fa90  Confidence: exact. Accept: the working copy becomes Summoner_options and summoner.cfg.
void options_apply_and_save()
{
    Summoner_options = options_edit;
    summoner_cfg_save();
    options_apply_detail();
    if (Summoner_options.cfg_3c)
        pxo_cfg_flags = (pxo_cfg_flags & ~2) | 1;
    else
        pxo_cfg_flags = (pxo_cfg_flags & ~1) | 2;
}

// 0x41fae0  Confidence: exact. Cancel: the sliders changed the volumes live, so put them back.
void options_restore_volumes()
{
    snd_set_group_volume(0, Summoner_options.sfx_volume);
    music_set_volume(Summoner_options.music_volume);
}

// 0x41fb00  Confidence: exact.
int options_frame(int key)
{
    int i;

    for (i = 0; i < OPTIONS_NUM_BUTTONS; i++)
        options_buttons[i].flags &= ~2;

    switch (options_mouse_mode) {
    case 0:
        key = options_handle_click(key);
        break;
    case 1:
        options_update_buttons();
        break;
    case 2:
        options_drag_sliders();
        break;
    }
    return key;
}

// 0x41fb50  Confidence: exact.
int options_handle_click(int key)
{
    int i;

    options_pressed_button = -1;

    for (i = 0; i < OPTIONS_NUM_BUTTONS; i++) {
        ui_button_def *d = &options_button_defs[i];
        ui_button_state *b = &options_buttons[i];
        if (!(b->flags & 4) && (d->flags & 7)) {
            int margin = 0;

            if (d->flags & 4)
                margin = options_slider_hit_margin[resolution_index];
            if (point_in_rect(game_mouse_x, game_mouse_y, b->x - margin / 2, b->y, b->w + margin, b->h)) {
                options_pressed_button = i;
                b->flags |= 2;
                if (d->flags & 3) {
                    if (mouse_was_pressed(0)) {
                        options_mouse_mode = 1;
                        return key;
                    }
                } else if (d->flags & 4) {
                    if (mouse_was_pressed(0)) {
                        options_mouse_mode = 2;
                        return key;
                    }
                }
                break;
            }
        }
    }

    switch (key) {
    case KEY_ESC:
        options_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        return 0;
    }
    return key;
}

// 0x41fc50  Confidence: exact. Back to the main menu (14) when opened from there, else to the game (0).
void options_close()
{
    if (options_flags & 2)
        interface_set_screen(14, 0);
    else
        interface_set_screen(0, 0);
}

// 0x41fc80  Confidence: exact. A HiLite entry (flag 1) acts through the Press entry after it.
// The table flags are read before the range test, as in the binary.
void options_update_buttons()
{
    int pressed = options_pressed_button;
    int button = pressed + 1;

    if (!(options_button_defs[pressed].flags & 1))
        button = pressed;

    if (pressed >= 0 && pressed < OPTIONS_NUM_BUTTONS) {
        ui_button_state *b = &options_buttons[button];
        if (point_in_rect(game_mouse_x, game_mouse_y, b->x, b->y, b->w, b->h))
            b->flags |= 2;

        if (mouse_is_down(0))
            return;
        if (b->flags & 2)
            options_button_action(button);
        if (ui_choice_popup_is_active())
            return;
    }
    options_mouse_mode = 0;
}

// 0x41fd20  Confidence: exact (VC6 byte match, match24; was likely) (byte table 0x41ff9c over button-4, 0..70, into the jump table 0x41ff0c).
// Every box case has its own sound call; VC6 cross-jumped the identical tails into the last copy (cfg_34
// off), which also explains why that tail sits before the accept / cancel cases. The case order is the
// code order (the combat flag cases come after cfg_3c). Sounds: 0x28 boxes, 0x23 accept, 0x24 cancel.
void options_button_action(int button)
{
    switch (button) {
    case 4:
        options_set_eax(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 6:
        options_set_eax(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 8:
        options_set_solo_confirm(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 10:
        options_set_solo_confirm(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 16:
        options_set_invert_camera(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 18:
        options_set_invert_camera(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 21:
        options_set_dialogue_camera(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 23:
        options_set_dialogue_camera(1);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 25:
        options_set_dialogue_camera(2);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 27:
        options_set_cfg_3c(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 29:
        options_set_cfg_3c(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 12:
        options_set_combat_flag(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 14:
        options_set_combat_flag(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 31:
        options_set_party_model_detail(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 33:
        options_set_party_model_detail(1);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 35:
        options_set_party_model_detail(2);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 37:
        options_set_model_detail(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 39:
        options_set_model_detail(1);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 41:
        options_set_model_detail(2);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 43:
        options_set_texture_quality(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 45:
        options_set_texture_quality(2);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 47:
        options_set_lightmaps_off(!options_edit.lightmaps_off);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 49:
        options_set_cfg_21(!options_edit.cfg_21);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 51:
        options_set_detail_24(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 53:
        options_set_detail_24(1);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 55:
        options_set_detail_24(2);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 57:
        options_set_fog_quality(0);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 59:
        options_set_fog_quality(1);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 61:
        options_set_fog_quality(2);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 63:
        options_set_antialias(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 65:
        options_set_antialias(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 67:
        options_set_cfg_34(true);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 69:
        options_set_cfg_34(false);
        game_play_sound(0x28, 1.0f, 0, 0, 0);
        break;
    case 72:
        options_flags |= 4;
        options_close();
        game_play_sound(0x23, 1.0f, 0, 0, 0);
        break;
    case 74:
        options_close();
        game_play_sound(0x24, 1.0f, 0, 0, 0);
        break;
    }
}

// 0x41fff0  Confidence: exact (VC6 byte match, D23603) (byte table 0x4202cc over pressed-1, 0..69, into
// 0x4202b8). The slider width is converted to a float local w for the clamp; the division converts the
// width again (8966 spills that conversion and copies it into w, the "copied once more" of earlier notes);
// the value is rounded to steps of 0.1. Any other pressed entry ends the drag at once.
// D23603: w declared in each case block (sets its slot per case) and the divisor written as the conversion.
void options_drag_sliders()
{
    float x, v;

    switch (options_pressed_button) {
    case 1: {
        x = (float)(game_mouse_x - options_sfx_slider_pos[resolution_index][0]);
        if (x <= 0.0f)
            x = 0.0f;
        float w = (float)options_sfx_slider_size[resolution_index][0];
        if (x >= w)
            x = w;
        v = (int)((x / (float)options_sfx_slider_size[resolution_index][0] + 0.05f) * 10.0f) * 0.1f;
        if (options_edit.sfx_volume != v)
            options_set_sfx_volume(v);
        break;
    }
    case 2: {
        x = (float)(game_mouse_x - options_music_slider_pos[resolution_index][0]);
        if (x <= 0.0f)
            x = 0.0f;
        float w = (float)options_music_slider_size[resolution_index][0];
        if (x >= w)
            x = w;
        v = (int)((x / (float)options_music_slider_size[resolution_index][0] + 0.05f) * 10.0f) * 0.1f;
        if (options_edit.music_volume != v)
            options_set_music_volume(v);
        break;
    }
    case 19: {
        x = (float)(game_mouse_x - options_camera_slider_pos[resolution_index][0]);
        if (x <= 0.0f)
            x = 0.0f;
        float w = (float)options_camera_slider_size[resolution_index][0];
        if (x >= w)
            x = w;
        v = (int)((x / (float)options_camera_slider_size[resolution_index][0] + 0.05f) * 10.0f) * 0.1f + 1.0f;
        if (options_edit.camera_speed != v)
            options_set_camera_speed(v);
        break;
    }
    case 70: {
        x = (float)(game_mouse_x - options_gamma_slider_pos[resolution_index][0]);
        if (x <= 0.0f)
            x = 0.0f;
        float w = (float)options_gamma_slider_size[resolution_index][0];
        if (x >= w)
            x = w;
        // Two statements: in one, VC6 folds 0.1f * 1.5f into 0.15f (D9801).
        v = (int)((x / (float)options_gamma_slider_size[resolution_index][0] + 0.05f) * 10.0f) * 0.1f;
        v = v * 1.5f + 0.7f;
        if (options_edit.gamma_correction != v)
            options_apply_gamma(v);
        break;
    }
    default:
        options_mouse_mode = 0;
        return;
    }

    if (!mouse_is_down(0))
        options_mouse_mode = 0;
}

// 0x420320  Confidence: exact (VC6 byte match, match9; was likely: order of the two antialias tests: 63 is compared
// first). Without card
// support the "antialias on" box (62) and its hot spot (63) are not drawn. Mode 3 is never set in this
// unit; the popup draw is reached only if another unit sets options_mouse_mode.
void options_draw()
{
    int i;

    for (i = 0; i < OPTIONS_NUM_BUTTONS; i++) {
        if (!gr_antialias_enabled && (i == 63 || i == 62))
            continue;

        switch (i) {
        case 1:
            options_draw_sfx_slider();
            break;
        case 2:
            options_draw_music_slider();
            break;
        case 19:
            options_draw_camera_speed_slider();
            break;
        case 70:
            options_draw_gamma_slider();
            break;
        }

        ui_button_state *b = &options_buttons[i];
        if (b->flags & 3) {
            gr_set_color(255, 255, 255, 255);
            ui_bitmap_scaled(b->bitmap, b->x, b->y, gr_bitmap_wrap_mode);
        }
    }

    if (options_mouse_mode == 3)
        ui_choice_popup_draw();
    options_draw_version();
}

// 0x420430  Confidence: exact (VC6 byte match, match9; was likely: operand order of the fill width). The filled part
// ends under the knob centre.
void options_draw_sfx_slider()
{
    int w = options_buttons[1].w / 2 - options_sfx_slider_pos[resolution_index][0] + options_buttons[1].x;
    int x = options_sfx_slider_pos[resolution_index][0];
    int y = options_sfx_slider_pos[resolution_index][1];
    int h = options_sfx_slider_size[resolution_index][1];

    if (w > 0) {
        gr_set_color_ptr(&ui_color_dark_green);
        gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
    }
}

// 0x420490  Confidence: exact (VC6 byte match, match9; was likely: as options_draw_sfx_slider).
void options_draw_music_slider()
{
    int w = options_buttons[2].w / 2 - options_music_slider_pos[resolution_index][0] + options_buttons[2].x;
    int x = options_music_slider_pos[resolution_index][0];
    int y = options_music_slider_pos[resolution_index][1];
    int h = options_music_slider_size[resolution_index][1];

    if (w > 0) {
        gr_set_color_ptr(&ui_color_dark_green);
        gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
    }
}

// 0x4204f0  Confidence: exact (VC6 byte match, match9; was likely: as options_draw_sfx_slider).
void options_draw_camera_speed_slider()
{
    int w = options_buttons[19].w / 2 - options_camera_slider_pos[resolution_index][0] + options_buttons[19].x;
    int x = options_camera_slider_pos[resolution_index][0];
    int y = options_camera_slider_pos[resolution_index][1];
    int h = options_camera_slider_size[resolution_index][1];

    if (w > 0) {
        gr_set_color_ptr(&ui_color_dark_green);
        gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
    }
}

// 0x420550  Confidence: exact (VC6 byte match, match9; was likely: as options_draw_sfx_slider).
void options_draw_gamma_slider()
{
    int w = options_buttons[70].w / 2 - options_gamma_slider_pos[resolution_index][0] + options_buttons[70].x;
    int x = options_gamma_slider_pos[resolution_index][0];
    int y = options_gamma_slider_pos[resolution_index][1];
    int h = options_gamma_slider_size[resolution_index][1];

    if (w > 0) {
        gr_set_color_ptr(&ui_color_dark_green);
        gr_rect(x, y, w, h, gr_rect_mode, 1.0f);
    }
}
