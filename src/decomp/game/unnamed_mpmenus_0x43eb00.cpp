// unnamed_mpmenus_0x43eb00.cpp - Sum.exe v1.40, code 0x43eb00-0x43ef24: the multiplayer menu states (state
// 14 level select, 13 game creation, 12 game list, the shared frame of states 10 and 13), the level
// availability checks of the level select screen, mp_host_game and the all_multi_levels console command.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl block, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: follows summoner_cd.cpp (cd_prompt_disc2 0x43ea20, its last strings.tbl id 937) and ends before the
// lobby unit's first initialiser (0x43ef30). Data: .data 0x594f88-0x594fcb (the command's literals only;
// "Usage: %s [bool]...", "%s is %s\n", "TRUE" / "FALSE" are pooled copies of earlier units, /Gf), .bss
// 0x60ae58-0x60ae64. See NOTES.md.

#include <string.h>
#include <stdio.h>
#include "vsdk/os/console.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/vector.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "summoner/multi/multi.h"

#define MP_LEVEL_TABLE_SIZE 26        // level_table entries (the loop ends at &level_table[26].mp_flags)
#define MP_DIFFICULTY_SHIFT 26        // the completed difficulties in the top bits of the .plr flags

// ---- .bss (0x60ae58) -----------------------------------------------------------------------------------
// The command object is defined at its initialiser, before cmd_all_multi_levels (below).
bool All_multi_levels;                                        // 0x60ae64

// 0x43eb00  Confidence: exact. State 14 (level select): leaves the lobby and opens screen 18.
void state14_enter()
{
    lobby_disconnect();
    mp_join_timer_reset();
    level_exit_trigger = NULL;
    game_frozen = false;
    interface_set_screen(18, 0);
}

// 0x43eb30  Confidence: exact. state_mp_menu_frame plus the chat history (the level select screen's chat).
void state14_frame()
{
    cursor_visible = true;
    interface_set_view_clip();
    gr_clear();
    chat_history_pump();
    interface_frame(PollInputEvents());
    interface_draw();
}

// 0x43eb60  Confidence: exact (a function of its own whose only statement is a call of an empty function:
// "jmp empty_function"; the callee's name is not known, empty_function is the folded copy).
void state14_leave()
{
    empty_function();
}

// 0x43eb70  Confidence: exact (the loop runs a pointer over level_table[i].mp_flags; written with the
// index). The completed difficulties (mask >> 26, signed) at or above 1 << difficulty open every level;
// else every level whose mp_flags share a bit with the level's prerequisite word (+0xc) must be completed.
bool mp_player_level_available(int mask, int level, int difficulty)
{
    int i;
    int need;

    if ((mask >> MP_DIFFICULTY_SHIFT) >= (1 << difficulty))
        return true;
    need = level_table[level].unk0c;
    for (i = 0; i < MP_LEVEL_TABLE_SIZE; i++) {
        if ((level_table[i].mp_flags & need) && !(mask & (1 << i)))
            return false;
    }
    return true;
}

// 0x43ebd0  Confidence: exact.
bool mp_level_available_to_all(int level, int difficulty)
{
    mp_player *p;

    if (All_multi_levels)
        return true;
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (!mp_player_level_available(p->profile_flags, level, difficulty))
            return false;
    }
    return true;
}

// 0x43ec20  Confidence: exact.
void mp_levelselect_set_level(int level)
{
    mp_netgame_level = level;
    multi_set_netgame_state(false);
}

// 0x43ec40  Confidence: exact. The host starts the level once every player has status 3.
void mp_levelselect_start_if_ready()
{
    if (mp_level_available_to_all(mp_netgame_level, mp_game_difficulty) && MP_IS_HOST()
        && mp_all_players_have_status(3))
        mp_change_level(-1);
}

// 0x43ec90  init_ccmd_all_multi_levels_thunk (static_initializers entry 146, jmp). Confidence: exact
// (compiler-generated).
// 0x43eca0  init_ccmd_all_multi_levels: console_command::console_command("all_multi_levels", ...).
// Confidence: exact (compiler-generated).
console_command ccmd_all_multi_levels("all_multi_levels", "Toggles All_multi_levels", cmd_all_multi_levels);   // 0x60ae58

// 0x43ecc0  Confidence: exact. The console's bool command macro (as cmd_map_mask in new_automap.cpp).
void cmd_all_multi_levels()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            All_multi_levels = true;
        else if (arg_type & ARG_FALSE)
            All_multi_levels = false;
        else if (arg_type & ARG_NONE)
            All_multi_levels ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "all_multi_levels", "All_multi_levels");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "all_multi_levels", All_multi_levels ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x43ed80  Confidence: exact. State 13: the game creation screen.
void state_mp_gamecreate_enter()
{
    interface_set_screen(17, 0);
}

// 0x43ed90  Confidence: exact. The frame of states 10 and 13.
void state_mp_menu_frame()
{
    cursor_visible = true;
    interface_set_view_clip();
    gr_clear();
    interface_frame(PollInputEvents());
    interface_draw();
}

// 0x43edc0  Confidence: exact (VC6 8966 byte match, m-1020 w6; was likely) (the max-players clamp is `>= 4` (jl past the store of 4); the netgame
// block is cleared as one object, D3305). The host's session: host bit, player id 0, status 3, the game
// settings, session state 1, the level scale and the tracker listing.
void mp_host_game(const char *name, const char *password, int max_players, int difficulty,
                  int levels_below, int levels_above)
{
    mp_local_player->flags = 1;
    mp_local_player->id = 0;
    mp_set_local_status(MP_STATUS_LISTED);
    memset(&mp_netgame, 0, sizeof(mp_netgame));
    strcpy(mp_netgame_name, name);
    strcpy(mp_netgame_password, password);
    mp_netgame_level = 0;
    mp_netgame_num_players = 1;
    mp_netgame_max_players = max_players < MP_MAX_PLAYERS ? max_players : MP_MAX_PLAYERS;
    mp_game_difficulty = difficulty;
    mp_netgame_level_below = levels_below;
    mp_netgame_level_above = levels_above;
    mp_netgame_state = 0;
    mp_set_session_state(1);
    mp_server_player_id = mp_local_player->id;
    mp_netgame_level_scale = frand_range(0.8f, 1.2f);
    if (pxo_cfg_flags & 1)
        tracker_start_game();
}

// 0x43eed0  Confidence: exact. State 12: the game list (a PXO game arrives here with the local player
// already made by the lobby).
void state_mp_gamelist_enter()
{
    if (!(pxo_cfg_flags & 1))
        mp_local_player_create();
    mp_gamelist_init();
    interface_set_screen(15, 0);
}

// 0x43eef0  Confidence: exact.
void state_mp_gamelist_frame()
{
    cursor_visible = true;
    interface_set_view_clip();
    gr_clear();
    mp_gamelist_net_frame();
    interface_frame(PollInputEvents());
    interface_draw();
}

// 0x43ef20  Confidence: exact ("jmp mp_gamelist_free").
void state_mp_gamelist_leave()
{
    mp_gamelist_free();
}
