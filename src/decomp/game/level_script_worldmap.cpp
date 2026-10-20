// level_script_worldmap.cpp - Sum.exe v1.40, code 0x4fb4d0-0x4fd6e1 (levelscripts\Scripts\
// level_script_worldmap.cpp; strings.tbl id 568 is this unit's, docs/sum-exe/source-map.md).
// Written from the Sum.exe disassembly (tools/asm.mjs; tables and floats read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (same, but one detail of statement order or expression form is a guess), unsure (the shape
// itself is a guess).
//
// The world map (worldmap1, level index 4): the location exits by game stage, the saved map position, the
// random encounter roll and the four encounter pickers, and the console commands "iw" and
// "random_encounters". Static initialiser table entries 492-495 (0x5887b4-0x5887c0) are this unit's.
// Data: .data 0x5a4c18-0x5a4f8c (the encounter level list, its count, Random_encounters_enabled, then the
// literals); .bss 0x257ed08-0x257ed5b. Pooled literals of other units: "worldmap1" 0x5945ec, the location
// level names ("lenele3d" 0x594728, "masad" 0x5930c0, "khosanistrng" 0x5a2098, ...), the rand-* level
// names ("rand-hills01" 0x5a0048, "rand-forest01" 0x5a0014, ...), the flag names of other levels
// ("got_ring_of_fire" 0x59ed9c, "unhide_korel" 0x5a1a20, ...) and the item names.
//
// Header (D1400): no header of its own; the hooks are in levelscripts/level_scripts.h, the helpers are
// declared below. Nothing outside the unit uses its globals (tools/asm.mjs grep).

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void ls_worldmap1_setup_lenele();
void ls_worldmap1_setup_masad();
void ls_worldmap1_setup_iona();
void ls_worldmap1_setup_ikaemos();
void ls_worldmap1_setup_khosani();
void ls_worldmap1_setup_jadetemple();
void ls_worldmap1_setup_wolong();
void ls_worldmap1_setup_liangshan();
bool ls_worldmap1_roll_encounter();
void ls_worldmap1_pick_encounter_early();
navpoint *ls_worldmap1_nearest_hills_nav(float *out_dist);
navpoint *ls_worldmap1_nearest_forest_nav(float *out_dist);
navpoint *ls_worldmap1_nearest_grass_nav(float *out_dist);
void ls_worldmap1_pick_encounter_stage4();
void ls_worldmap1_pick_encounter_stage9();
navpoint *ls_worldmap1_nearest_desert_nav(float *out_dist);
navpoint *ls_worldmap1_nearest_orenian_nav(float *out_dist);
void ls_worldmap1_pick_encounter_stage15();
void cmd_iw();
void cmd_random_encounters();

// ---- .data (0x5a4c18-) -------------------------------------------------------------------------------
// Coming back from one of these levels restores the saved map position. Names ours.
char *ls_worldmap1_encounter_levels[12] = {                  // 0x5a4c18
    "Rand-Forest01", "Rand-Grassland01", "Rand-Iceland01", "Rand-Hills01", "Rand-Desert", "Rand-Orenia01",
    "Rand-ForestNite1", "Rand-GrasslandNite01", "Rand-IcelandNite01", "Rand-HillsNite01", "Rand-DesertNite01",
    "Rand-OreniaNite01",
};
int ls_worldmap1_num_encounter_levels = 12;                  // 0x5a4c48 (re-read every iteration)
bool Random_encounters_enabled = true;                       // 0x5a4c4c

// ---- .bss (0x257ed08-0x257ed5b) ----------------------------------------------------------------------
// The objects with dynamic initialisers are defined in initialiser order (entries 492-495); their .bss
// order is timer 0x257ed08, ccmd_random_encounters 0x257ed10, ls_worldmap1_start 0x257ed20, ccmd_iw
// 0x257ed50 (0x257ed0c is never referenced). See NOTES.

// 0x4fb4d0  Confidence: likely. static_init_empty_4fb4e0: "jmp 0x4fb4e0" (initialiser entry 492).
// 0x4fb4e0  Confidence: likely. init_empty_4fb4e0: a bare ret, the implicit constructor of a navpoint (its
// vector3 has an empty inline constructor). The saved-position marker every level_player_starts slot points
// at after an encounter; sum.symbols' ls_worldmap1_heading (0x257ed30) is its +0x10 heading. Name ours.
navpoint ls_worldmap1_start;                                 // 0x257ed20

// 0x4fb4f0  Confidence: exact. static_init_ls_worldmap1_timer: "jmp 0x4fb500" (initialiser entry 493).
// 0x4fb500  Confidence: exact. init_ls_worldmap1_timer: "mov ecx, 0x257ed08 / jmp GameTimer::GameTimer".
GameTimer ls_worldmap1_encounter_timer;                      // 0x257ed08

// 0x4fb510  Confidence: exact (VC6 byte match, match24; was likely) (the discarded first game_stage_get call). level_script_table init.
void ls_worldmap1_init()
{
    Character *c;
    int i;

    game_stage_get();
    for (i = 0; i < ls_worldmap1_num_encounter_levels; i++) {
        if (!_stricmp(level_prev_filename, ls_worldmap1_encounter_levels[i])) {
            ls_worldmap1_start.pos.x = savegame_state.get_float(cur_level_index, "worldmap_last_pos_x");
            ls_worldmap1_start.pos.y = savegame_state.get_float(cur_level_index, "worldmap_last_pos_y");
            ls_worldmap1_start.pos.z = savegame_state.get_float(cur_level_index, "worldmap_last_pos_z");
            ls_worldmap1_start.layer = savegame_state.get_int(cur_level_index, "worldmap_last_pos_plane");
            ls_worldmap1_start.heading = savegame_state.get_float(cur_level_index, "worldmap_last_pos_heading");
            level_player_starts[0] = &ls_worldmap1_start;
            level_player_starts[1] = &ls_worldmap1_start;
            level_player_starts[2] = &ls_worldmap1_start;
            level_player_starts[3] = &ls_worldmap1_start;
            break;
        }
    }
    ls_worldmap1_setup_lenele();
    ls_worldmap1_setup_masad();
    ls_worldmap1_setup_iona();
    ls_worldmap1_setup_ikaemos();
    ls_worldmap1_setup_khosani();
    ls_worldmap1_setup_jadetemple();
    ls_worldmap1_setup_wolong();
    ls_worldmap1_setup_liangshan();
    ls_worldmap1_encounter_timer.SetTimeout(20000);
    level_music = music_find_by_name("worldmap1");
    cur_character->set_speed_scale((cur_character->walk_speed + cur_character->run_speed) * 0.5f, true);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7 && c != cur_character) {
            c->set_ai_active(false);
            c->flags |= CF_HIDDEN;
        } else if (c->type == 7 && c == cur_character) {
            c->set_ai_active(true);
            c->flags &= ~CF_HIDDEN;
        }
    }
}

// ---- the location exits: one trigger per location gets level_trigger_set_exit -----------------------

// 0x4fb6d0  Confidence: exact (the 15+, 7+ and < 4 cases share the last two calls).
void ls_worldmap1_setup_lenele()
{
    if (game_stage_get() >= 15) {
        level_trigger_set_exit("lenele3d", NULL);
        level_trigger_set_script_use("lenele2d", NULL);
        level_trigger_set_script_use("lenele1d", "lenele1d");
        level_trigger_set_script_use("lenele1d", "lenele1d_v2");
        level_trigger_set_script_use("lenele1d", "lenele1d_v3");
    } else if (game_stage_get() >= 7) {
        level_trigger_set_script_use("lenele3d", NULL);
        level_trigger_set_exit("lenele2d", NULL);
        level_trigger_set_script_use("lenele1d", "lenele1d");
        level_trigger_set_script_use("lenele1d", "lenele1d_v2");
        level_trigger_set_script_use("lenele1d", "lenele1d_v3");
    } else if (game_stage_get() >= 6) {
        level_trigger_set_script_use("lenele3d", NULL);
        level_trigger_set_script_use("lenele2d", NULL);
        level_trigger_set_script_use("lenele1d", "lenele1d");
        level_trigger_set_script_use("lenele1d", "lenele1d_v2");
        level_trigger_set_exit("lenele1d", "lenele1d_v3");
    } else if (game_stage_get() >= 4) {
        level_trigger_set_script_use("lenele3d", NULL);
        level_trigger_set_script_use("lenele2d", NULL);
        level_trigger_set_script_use("lenele1d", "lenele1d");
        level_trigger_set_exit("lenele1d", "lenele1d_v2");
        level_trigger_set_script_use("lenele1d", "lenele1d_v3");
    } else {
        level_trigger_set_script_use("lenele3d", NULL);
        level_trigger_set_script_use("lenele2d", NULL);
        level_trigger_set_exit("lenele1d", "lenele1d");
        level_trigger_set_script_use("lenele1d", "lenele1d_v2");
        level_trigger_set_script_use("lenele1d", "lenele1d_v3");
    }
}

// 0x4fb830  Confidence: exact. Only the masad_v2 trigger is touched.
void ls_worldmap1_setup_masad()
{
    if (game_stage_get() >= 9) {
        level_trigger_set_script_use("masad", "masad_v2");
        lobjs_hide_by_prefix("Masad");
    } else
        level_trigger_set_exit("masad", "masad_v2");
}

// 0x4fb860  Confidence: exact.
void ls_worldmap1_setup_iona()
{
    if (game_stage_get() >= 16) {
        level_trigger_set_script_use("ionaext", NULL);
        level_trigger_set_script_use("ionaext02", "ionaext02");
        level_trigger_set_exit("ionaext02", "ionaext02_v2");
    } else if (game_stage_get() >= 9) {
        level_trigger_set_script_use("ionaext", NULL);
        level_trigger_set_exit("ionaext02", "ionaext02");
        level_trigger_set_script_use("ionaext02", "ionaext02_v2");
    } else if (game_stage_get() >= 2) {
        level_trigger_set_exit("ionaext", NULL);
        level_trigger_set_script_use("ionaext02", "ionaext02");
        level_trigger_set_script_use("ionaext02", "ionaext02_v2");
    } else {
        level_trigger_set_script_use("ionaext", NULL);
        level_trigger_set_script_use("ionaext02", "ionaext02");
        level_trigger_set_script_use("ionaext02", "ionaext02_v2");
        lobjs_hide_by_prefix("Iona");
    }
}

// 0x4fb940  Confidence: exact. got_ikaemos_map is read from level 5's block.
void ls_worldmap1_setup_ikaemos()
{
    if (game_stage_get() >= 9) {
        level_trigger_set_script_use("ikaemosext", NULL);
        level_trigger_set_exit("ikaemosext2", NULL);
    } else if (savegame_state.get_flag(5, "got_ikaemos_map")) {
        level_trigger_set_exit("ikaemosext", NULL);
        level_trigger_set_script_use("ikaemosext2", NULL);
    } else {
        level_trigger_set_script_use("ikaemosext", NULL);
        level_trigger_set_script_use("ikaemosext2", NULL);
        lobjs_hide_by_prefix("Ikaemos");
    }
}

// 0x4fb9c0  Confidence: exact. got_ring_of_fire is read from level 14, found_khosani from the world map.
void ls_worldmap1_setup_khosani()
{
    if (game_stage_get() >= 20) {
        level_trigger_set_script_use("khosanistrng", "khosanistrng");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v2");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v3");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v4");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v5");
        level_trigger_set_exit("khosanistrng", "khosanistrng_v6");
    } else if (game_stage_get() >= 18) {
        level_trigger_set_script_use("khosanistrng", "khosanistrng");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v2");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v3");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v4");
        level_trigger_set_exit("khosanistrng", "khosanistrng_v5");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v6");
    } else if (game_stage_get() >= 17) {
        level_trigger_set_script_use("khosanistrng", "khosanistrng");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v2");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v3");
        level_trigger_set_exit("khosanistrng", "khosanistrng_v4");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v5");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v6");
    } else if (game_stage_get() >= 9) {
        level_trigger_set_script_use("khosanistrng", "khosanistrng");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v2");
        level_trigger_set_exit("khosanistrng", "khosanistrng_v3");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v4");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v5");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v6");
    } else if (savegame_state.get_flag(14, "got_ring_of_fire")) {
        level_trigger_set_script_use("khosanistrng", "khosanistrng");
        level_trigger_set_exit("khosanistrng", "khosanistrng_v2");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v3");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v4");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v5");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v6");
    } else if (savegame_state.get_flag(cur_level_index, "found_khosani")) {
        level_trigger_set_exit("khosanistrng", "khosanistrng");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v2");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v3");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v4");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v5");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v6");
    } else {
        level_trigger_set_script_use("khosanistrng", "khosanistrng");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v2");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v3");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v4");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v5");
        level_trigger_set_script_use("khosanistrng", "khosanistrng_v6");
        lobjs_hide_by_prefix("Khosani");
    }
}

// 0x4fbcb0  Confidence: exact. The eleh trigger is never an exit here.
void ls_worldmap1_setup_jadetemple()
{
    if (game_stage_get() >= 14) {
        level_trigger_set_script_use("eleh", NULL);
        level_trigger_set_exit("jadetemple", NULL);
    } else {
        level_trigger_set_script_use("eleh", NULL);
        level_trigger_set_script_use("jadetemple", NULL);
        lobjs_hide_by_prefix("JTE");
    }
}

// 0x4fbd00  Confidence: exact.
void ls_worldmap1_setup_wolong()
{
    if (game_stage_get() >= 13) {
        level_trigger_set_script_use("wolong", NULL);
        level_trigger_set_exit("wolong2", "wolong2_v2");
    } else {
        level_trigger_set_exit("wolong", NULL);
        level_trigger_set_script_use("wolong2", "wolong2_v2");
    }
}

// 0x4fbd50  Confidence: exact.
void ls_worldmap1_setup_liangshan()
{
    if (game_stage_get() >= 13)
        level_trigger_set_exit("liangshan", NULL);
    else {
        level_trigger_set_script_use("liangshan", NULL);
        lobjs_hide_by_prefix("Liangshan");
    }
}

// ---- random encounters -----------------------------------------------------------------------------

// 0x4fbd80  Confidence: exact (the position floats go through a parenthesised float temporary, fld / fstp
// into the argument slot, D10402 / D11303). level_script_table frame. On an encounter the leader's position, plane and
// heading are saved, a picker sets level_filename / level_script_name, and the level is changed.
void ls_worldmap1_frame()
{
    float h, b, p;

    if (ls_worldmap1_roll_encounter() == true) {
        hud_show_message(XSTR("Encounter!", 568));
        cur_character->orient.extract_angles(&p, &b, &h);
        ls_worldmap1_start.heading = h;
        {
            float t = (cur_character->pos.x);
            savegame_state.set_float(cur_level_index, "worldmap_last_pos_x", t);
        }
        {
            float t = (cur_character->pos.y);
            savegame_state.set_float(cur_level_index, "worldmap_last_pos_y", t);
        }
        {
            float t = (cur_character->pos.z);
            savegame_state.set_float(cur_level_index, "worldmap_last_pos_z", t);
        }
        savegame_state.set_int(cur_level_index, "worldmap_last_pos_plane", cur_character->layer);
        savegame_state.set_float(cur_level_index, "worldmap_last_pos_heading", h);
        if (game_stage_get() >= 15)
            ls_worldmap1_pick_encounter_stage15();
        else if (game_stage_get() >= 9)
            ls_worldmap1_pick_encounter_stage9();
        else if (game_stage_get() >= 4)
            ls_worldmap1_pick_encounter_stage4();
        else
            ls_worldmap1_pick_encounter_early();
        printf("loading %s\n", level_script_name);
        level_entry_point = 1;
        gameseq_set_state(4);
    }
}

// 0x4fbed0  Confidence: exact (VC6 byte match, match9; was likely: the stone case returns at once where the light and
// fire cases zero the
// roll; the code cannot tell `return true` from a third `roll = 0` the compiler folded). A check every
// 10 s while moving; the boss rings force the encounter until the boss intro has played.
bool ls_worldmap1_roll_encounter()
{
    float roll;

    if (!Random_encounters_enabled)
        return false;
    if (!ls_worldmap1_encounter_timer.elapsed())
        return false;
    ls_worldmap1_encounter_timer.SetTimeout(10000);
    if (cur_character->get_698() == 0.0f)
        return false;
    if (Ai_paused)
        return false;
    roll = frand();
    if (savegame_state.get_flag(1, "got_ring_of_light") && !savegame_state.get_flag(19, "ghost_intro_played"))
        roll = 0.0f;
    if (savegame_state.get_flag(14, "got_ring_of_fire") && !savegame_state.get_flag(10, "phoenix_intro_played"))
        roll = 0.0f;
    if ((savegame_state.get_flag(12, "got_ring_of_stone") && !savegame_state.get_flag(18, "serpent_intro_played"))
        || roll <= 0.5f)
        return true;
    return false;
}

// 0x4fbfd0  Confidence: exact (VC6 byte match, match9; was likely: the Korel / Zane tails are merged in the code; the
// grass case is entered
// straight from the distance compare). Stage < 4. The navpoint results are not used, only the distances.
void ls_worldmap1_pick_encounter_early()
{
    float dist, best;
    float roll;
    int terrain;

    ls_worldmap1_nearest_hills_nav(&best);
    terrain = 0;
    ls_worldmap1_nearest_forest_nav(&dist);
    if (dist < best) {
        terrain = 1;
        best = dist;
    }
    ls_worldmap1_nearest_grass_nav(&dist);
    if (dist < best) {
        best = dist;
        terrain = 2;
    }
    if (terrain == 0) {
        if (frand() < 0.5f) {
            strcpy(level_filename, "rand-hills01");
            strcpy(level_script_name, "rand-hills01");
        } else {
            strcpy(level_filename, "rand-hillsnite01");
            strcpy(level_script_name, "rand-hillsnite01");
        }
    } else if (terrain == 1) {
        if (frand() < 0.5f) {
            strcpy(level_filename, "rand-forest01");
            strcpy(level_script_name, "rand-forest01");
        } else {
            strcpy(level_filename, "rand-forestnite1");
            strcpy(level_script_name, "rand-forestnite1");
        }
    } else if (terrain == 2) {
        if (savegame_state.get_flag(35, "unhide_korel") && !savegame_state.get_flag(42, "no_more_korel")) {
            printf("Korel active.\n");
            if (frand() < 0.5f) {
                strcpy(level_filename, "rand-grassland01");
                strcpy(level_script_name, "rand-grassland01_v3");
                return;
            }
        } else if (savegame_state.get_flag(7, "unhide_zane") && !savegame_state.get_flag(42, "no_more_zane")) {
            printf("Zane active.\n");
            if (frand() < 0.5f) {
                strcpy(level_filename, "rand-grassland01");
                strcpy(level_script_name, "rand-grassland01_v3");
                return;
            }
        }
        roll = frand();
        if (roll < 0.45f) {
            strcpy(level_filename, "rand-grassland01");
            strcpy(level_script_name, "rand-grassland01");
        } else if (roll < 0.55f) {
            strcpy(level_filename, "rand-grassland01");
            strcpy(level_script_name, "rand-grassland01_v2");
        } else {
            strcpy(level_filename, "rand-grasslandnite01");
            strcpy(level_script_name, "rand-grasslandnite01");
        }
    }
}

// 0x4fc2a0  Confidence: exact (VC6 byte match, match9; was likely: the distance expression; the code squares z, x, y
// in that order). The nearest
// "$hills..." navpoint to the leader and its distance (10000 when there is none).
navpoint *ls_worldmap1_nearest_hills_nav(float *out_dist)
{
    float best = 10000.0f;
    int found = -1;
    int i;

    for (i = 0; i < navpoint_count; i++) {
        if (!_strnicmp(navpoints[i].name, "$hills", 6)) {
            vector3 v = cur_character->pos - navpoints[i].pos;
            float d = v.len();
            if (d < best) {
                best = d;
                found = i;
            }
        }
    }
    *out_dist = best;
    if (found == -1)
        return NULL;
    return &navpoints[found];
}

// 0x4fc350  Confidence: exact (VC6 byte match, match9; was likely: as the hills search). "$forest".
navpoint *ls_worldmap1_nearest_forest_nav(float *out_dist)
{
    float best = 10000.0f;
    int found = -1;
    int i;

    for (i = 0; i < navpoint_count; i++) {
        if (!_strnicmp(navpoints[i].name, "$forest", 7)) {
            vector3 v = cur_character->pos - navpoints[i].pos;
            float d = v.len();
            if (d < best) {
                best = d;
                found = i;
            }
        }
    }
    *out_dist = best;
    if (found == -1)
        return NULL;
    return &navpoints[found];
}

// 0x4fc400  Confidence: exact (VC6 byte match, match9; was likely: as the hills search). "$grass".
navpoint *ls_worldmap1_nearest_grass_nav(float *out_dist)
{
    float best = 10000.0f;
    int found = -1;
    int i;

    for (i = 0; i < navpoint_count; i++) {
        if (!_strnicmp(navpoints[i].name, "$grass", 6)) {
            vector3 v = cur_character->pos - navpoints[i].pos;
            float d = v.len();
            if (d < best) {
                best = d;
                found = i;
            }
        }
    }
    *out_dist = best;
    if (found == -1)
        return NULL;
    return &navpoints[found];
}

// 0x4fc4b0  Confidence: exact (VC6 byte match, match24; was likely) (shared tails; the Zane case has no return and is overwritten by the general
// rolls below it, see NOTES). Stage 4..8: the ring bosses first (each also remembers the map in
// level_prev_filename), then by terrain.
void ls_worldmap1_pick_encounter_stage4()
{
    navpoint *nearest, *nav;
    float dist, best, roll, r;
    int terrain;

    if (savegame_state.get_flag(1, "got_ring_of_light") && !savegame_state.get_flag(19, "ghost_killed")) {
        strcpy(level_prev_filename, level_filename);
        strcpy(level_filename, "rand-forestnite1");
        strcpy(level_script_name, "rand-forestnite1_v2");
        return;
    }
    if (savegame_state.get_flag(14, "got_ring_of_fire") && !savegame_state.get_flag(10, "phoenix_killed")) {
        strcpy(level_prev_filename, level_filename);
        strcpy(level_filename, "rand-hills01");
        strcpy(level_script_name, "rand-hills01_v2");
        return;
    }
    if (savegame_state.get_flag(12, "got_ring_of_stone") && !savegame_state.get_flag(18, "serpent_killed")) {
        strcpy(level_prev_filename, level_filename);
        strcpy(level_filename, "rand-forest01");
        strcpy(level_script_name, "rand-forest01_v2");
        return;
    }
    nearest = ls_worldmap1_nearest_hills_nav(&best);
    terrain = 0;
    nav = ls_worldmap1_nearest_forest_nav(&dist);
    if (dist < best) {
        best = dist;
        nearest = nav;
        terrain = 1;
    }
    ls_worldmap1_nearest_grass_nav(&dist);
    if (dist < best) {
        best = dist;
        terrain = 2;
    }
    if (terrain == 0) {
        roll = frand();
        if (!_stricmp(nearest->name, "$hills001") && roll < 0.7f
            && !savegame_state.get_flag(cur_level_index, "found_khosani")) {
            strcpy(level_prev_filename, level_filename);
            strcpy(level_filename, "rand-hills01");
            strcpy(level_script_name, "rand-hills01_v3");
            return;
        }
        r = frand();
        if (r < 0.33f) {
            strcpy(level_filename, "rand-hillsnite01");
            strcpy(level_script_name, "rand-hillsnite01_v2");
        } else if (r < 0.66f) {
            strcpy(level_filename, "rand-hillsnite01");
            strcpy(level_script_name, "rand-hillsnite01_v3");
        } else {
            strcpy(level_filename, "rand-hills01");
            strcpy(level_script_name, "rand-hills01");
        }
    } else if (terrain == 1) {
        r = frand();
        if (r < 0.25f) {
            strcpy(level_filename, "rand-forest01");
            strcpy(level_script_name, "rand-forest01_v3");
        } else if (r < 0.5f) {
            strcpy(level_filename, "rand-forest01");
            strcpy(level_script_name, "rand-forest01_v4");
        } else if (r < 0.75f && game_stage_get() >= 7) {
            strcpy(level_filename, "rand-forestnite1");
            strcpy(level_script_name, "rand-forestnite1_v3");
        } else {
            strcpy(level_filename, "rand-forestnite1");
            strcpy(level_script_name, "rand-forestnite1");
        }
    } else if (terrain == 2) {
        roll = frand();
        if (savegame_state.get_flag(35, "unhide_korel") && !savegame_state.get_flag(42, "no_more_korel")) {
            if (frand() < 0.5f) {
                strcpy(level_filename, "rand-grassland01");
                strcpy(level_script_name, "rand-grassland01_v3");
                return;
            }
        } else if (savegame_state.get_flag(7, "unhide_zane") && !savegame_state.get_flag(42, "no_more_zane")
                   && frand() < 0.5f) {
            strcpy(level_filename, "rand-grassland01");
            strcpy(level_script_name, "rand-grassland01_v3");
        }
        if (roll < 0.1f) {
            strcpy(level_filename, "rand-grassland01");
            strcpy(level_script_name, "rand-grassland01_v3");
        } else if (roll < 0.55f) {
            strcpy(level_filename, "rand-grasslandnite01");
            strcpy(level_script_name, "rand-grasslandnite012");
        } else {
            strcpy(level_filename, "rand-grasslandnite01");
            strcpy(level_script_name, "rand-grasslandnite01");
        }
    }
}

// 0x4fcaf0  Confidence: likely (the shared suffix tail). Stage 9..14: desert or orenian by the nearer
// navpoint (desert only when strictly nearer), day or night, then an optional _v2 / _v3 suffix. One frand
// result is thrown away before the suffix rolls.
void ls_worldmap1_pick_encounter_stage9()
{
    float dist, best;

    ls_worldmap1_nearest_orenian_nav(&best);
    ls_worldmap1_nearest_desert_nav(&dist);
    if (dist < best) {
        best = dist;
        if (frand() < 0.5f) {
            strcpy(level_filename, "rand-desert");
            strcpy(level_script_name, "rand-desert");
        } else {
            strcpy(level_filename, "rand-desertnite01");
            strcpy(level_script_name, "rand-desertnite01");
        }
        frand();
        if (frand() < 0.35f)
            strcat(level_script_name, "_v2");
        else if (frand() < 0.65f)
            strcat(level_script_name, "_v3");
    } else {
        if (frand() < 0.5f) {
            strcpy(level_filename, "rand-orenia01");
            strcpy(level_script_name, "rand-orenia01");
        } else {
            strcpy(level_filename, "rand-orenianite01");
            strcpy(level_script_name, "rand-orenianite01");
        }
        frand();
        if (frand() < 0.45f)
            strcat(level_script_name, "_v2");
        else if (frand() < 0.55f)
            strcat(level_script_name, "_v3");
    }
}

// 0x4fcce0  Confidence: exact (VC6 byte match, match9; was likely: as the hills search). "$desert".
navpoint *ls_worldmap1_nearest_desert_nav(float *out_dist)
{
    float best = 10000.0f;
    int found = -1;
    int i;

    for (i = 0; i < navpoint_count; i++) {
        if (!_strnicmp(navpoints[i].name, "$desert", 7)) {
            vector3 v = cur_character->pos - navpoints[i].pos;
            float d = v.len();
            if (d < best) {
                best = d;
                found = i;
            }
        }
    }
    *out_dist = best;
    if (found == -1)
        return NULL;
    return &navpoints[found];
}

// 0x4fcd90  Confidence: exact (VC6 byte match, match9; was likely: as the hills search). "$orenian".
navpoint *ls_worldmap1_nearest_orenian_nav(float *out_dist)
{
    float best = 10000.0f;
    int found = -1;
    int i;

    for (i = 0; i < navpoint_count; i++) {
        if (!_strnicmp(navpoints[i].name, "$orenian", 8)) {
            vector3 v = cur_character->pos - navpoints[i].pos;
            float d = v.len();
            if (d < best) {
                best = d;
                found = i;
            }
        }
    }
    *out_dist = best;
    if (found == -1)
        return NULL;
    return &navpoints[found];
}

// 0x4fce40  Confidence: likely (shared suffix tails; the compares against the first roll are scheduled
// before the strcpys). Stage 15+: the Tome Page 8 quest first, then by terrain (hills gives the iceland
// maps).
void ls_worldmap1_pick_encounter_stage15()
{
    float dist, r2, best, r1;
    int terrain;

    ls_worldmap1_nearest_hills_nav(&best);
    terrain = 0;
    ls_worldmap1_nearest_forest_nav(&dist);
    if (dist < best) {
        terrain = 1;
        best = dist;
    }
    ls_worldmap1_nearest_grass_nav(&dist);
    if (dist < best) {
        terrain = 2;
        best = dist;
    }
    if (savegame_state.get_flag(18, "nhuvasarim_mq_bought_page7")
        && !party_count_item_by_name("Parchment#Tome Page 8")
        && !savegame_state.get_flag(34, "got_laharah_spirit")) {
        strcpy(level_filename, "rand-icelandnite01");
        strcpy(level_script_name, "rand-icelandnite01_v2");
        return;
    }
    if (terrain == 0) {
        if (savegame_state.get_flag(23, "khosanistrng_haenulquest") && !savegame_state.get_flag(2, "extra_5")
            && !party_count_item_by_name("Eraekor Gemstone")) {
            strcpy(level_filename, "rand-iceland01");
            strcpy(level_script_name, "rand-iceland01_v4");
            savegame_state.set_flag(2, "extra_5", true);
            return;
        }
        r1 = frand();
        if (r1 < 0.05f) {
            strcpy(level_filename, "rand-hills01");
            strcpy(level_script_name, "rand-hills01_v4");
        } else if (r1 < 0.5f) {
            strcpy(level_filename, "rand-iceland01");
            strcpy(level_script_name, "rand-iceland01");
            if (r1 < 0.16f)
                strcat(level_script_name, "_v2");
            else if (r1 < 0.33f)
                strcat(level_script_name, "_v3");
        } else {
            strcpy(level_filename, "rand-icelandnite01");
            strcpy(level_script_name, "rand-icelandnite01");
            if (r1 < 0.75f)
                strcat(level_script_name, "_v3");
        }
    } else if (terrain == 1) {
        r1 = frand();
        r2 = frand();
        if (party_count_item_by_name("Parchment#Tome Page 1") && party_count_item_by_name("Parchment#Tome Page 2")
            && party_count_item_by_name("Parchment#Tome Page 3") && party_count_item_by_name("Parchment#Tome Page 4")
            && party_count_item_by_name("Parchment#Tome Page 5") && party_count_item_by_name("Parchment#Tome Page 6")
            && !savegame_state.get_flag(34, "got_laharah_spirit")) {
            strcpy(level_filename, "rand-forest01");
            strcpy(level_script_name, "rand-forest01_v6");
        } else if (r1 < 0.5f) {
            strcpy(level_filename, "rand-forest01");
            strcpy(level_script_name, "rand-forest01");
            if (r2 < 0.2f)
                strcat(level_script_name, "_v5");
            else
                strcat(level_script_name, "_v6");
        } else {
            strcpy(level_filename, "rand-forestnite1");
            strcpy(level_script_name, "rand-forestnite1");
            if (r2 < 0.2f)
                strcat(level_script_name, "_v3");
            else if (r2 < 0.4f)
                strcat(level_script_name, "_v4");
            else if (r2 < 0.6f)
                strcat(level_script_name, "_v5");
            else if (r2 < 0.7f)
                strcat(level_script_name, "_v6");
            else
                strcat(level_script_name, "_v7");
        }
    } else if (terrain == 2) {
        r2 = frand();
        if (r2 < 0.1f && savegame_state.get_flag(23, "ready_for_end")
            && savegame_state.get_flag(34, "got_laharah_spirit")) {
            strcpy(level_filename, "rand-grassland01");
            strcpy(level_script_name, "rand-grassland01_v5");
        } else if (r2 < 0.5f && !savegame_state.get_flag(2, "extra_1")) {
            savegame_state.set_flag(2, "extra_1", true);
            strcpy(level_filename, "rand-grassland01");
            strcpy(level_script_name, "rand-grassland01_v4");
        } else {
            strcpy(level_filename, "rand-grasslandnite01");
            strcpy(level_script_name, "rand-grasslandnite013");
        }
    }
}

// 0x4fd560  Confidence: exact. level_script_table leave: the leader runs again, every party member is
// shown and active.
void ls_worldmap1_leave()
{
    Character *c;

    if (cur_character)
        cur_character->set_speed_scale(cur_character->run_speed, false);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7) {
            c->set_ai_active(true);
            c->flags &= ~CF_HIDDEN;
        }
    }
}

// 0x4fd5c0  Confidence: exact. static_init_ccmd_iw: "jmp 0x4fd5d0" (initialiser entry 494).
// 0x4fd5d0  Confidence: exact. init_ccmd_iw: console_command::console_command(this = 0x257ed50).
console_command ccmd_iw("iw", "reinits worldmap", cmd_iw);   // 0x257ed50

// 0x4fd5f0  Confidence: exact (a tail jump).
void cmd_iw()
{
    ls_worldmap1_init();
}

// 0x4fd600  Confidence: exact. static_init_ccmd_random_encounters: "jmp 0x4fd610" (initialiser entry 495).
// 0x4fd610  Confidence: exact. init_ccmd_random_encounters: console_command::console_command(this = 0x257ed10).
console_command ccmd_random_encounters("random_encounters", "Toggles Random_encounters_enabled",
                                       cmd_random_encounters);   // 0x257ed10

// 0x4fd630  Confidence: exact. The bool toggle pattern (as cmd_Preparse_scripts).
void cmd_random_encounters()
{
    if (calling_a_command_function) {
        process_command(0xc1);
        if (arg_type & ARG_TRUE)
            Random_encounters_enabled = true;
        else if (arg_type & ARG_FALSE)
            Random_encounters_enabled = false;
        else if (arg_type & ARG_NONE)
            Random_encounters_enabled ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "random_encounters", "Random_encounters_enabled");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "random_encounters", Random_encounters_enabled ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}
