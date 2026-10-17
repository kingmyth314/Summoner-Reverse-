// level_script_lenele1aa.cpp - Sum.exe v1.40, code 0x4efa30-0x4effa5 (levelscripts\Scripts\
// level_script_lenele1aa.cpp; strings.tbl id 500 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs; the short trigger names "d1", "b1".."b3" read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and
// globals match), likely (one detail of statement order or expression form is a guess), unsure (the shape
// itself is a guess).
//
// One level_script_table record: "lenele1aa" (the Lenele marketplace; the variants lenele1aa_v2 (stage 4)
// and lenele1aa_v3 (stage 6) select the triggers of the later visits). Jekhar's first meeting, the
// cartwright and Krys the Lantern Maker's gargoyle blood. No static initialiser, no .bss, no .data of its
// own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/localize/localize.h"
#include "engine/camera/camera.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_LPALACEINT   0x18                      // "got_ring_of_darkness" (0x4efa95)

void lsf_lenele_flag_cart_played();
void lsf_lenele1aa_jekhar_talk();

// 0x4efa30  Confidence: exact (the first visit's flag test written as an early return; the >= 4 and < 4
// branches each end in the eight _v3 script-use calls, which 8966 tail-merges: 0x4efd75 jmp 0x4efc83). level_script_table init.
// On the first visit, once Jekhar has talked or the Ring of Darkness is in hand, Jekhar is gone and
// nothing else is set up: no game stage and no triggers.
void ls_lenele1aa_init()
{
    level_music = music_find_by_name("lenele marketplace");
    if (!_stricmp(level_script_name, "lenele1aa_v3")) {
        game_stage_set(6);
    } else if (!_stricmp(level_script_name, "lenele1aa_v2")) {
        game_stage_set(4);
    } else {
        if (savegame_state.get_flag(cur_level_index, "lenele1aa_jekhar_talk")
            || savegame_state.get_flag(LEVEL_LPALACEINT, "got_ring_of_darkness")) {
            savegame_state.set_flag(cur_level_index, "lenele1aa_jekhar_talk", true);
            script_npc_set_disabled("Jekhar#$npc012", true);
            return;
        }
        game_stage_set(0);
    }

    if (game_stage_get() >= 1)
        level_trigger_set_exit_by_refname("sewer");
    else
        level_trigger_set_script_use_by_refname("sewer");

    if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("d1");
        level_trigger_set_script_use_by_refname("b1");
        level_trigger_set_script_use_by_refname("b2");
        level_trigger_set_script_use_by_refname("b3");
        level_trigger_set_script_use_by_refname("ab1");
        level_trigger_set_script_use_by_refname("ab2");
        level_trigger_set_script_use_by_refname("ab3");
        level_trigger_set_script_use_by_refname("ab4");
        level_trigger_set_script_use_by_refname("d1_v2");
        level_trigger_set_script_use_by_refname("b1_v2");
        level_trigger_set_script_use_by_refname("b2_v2");
        level_trigger_set_script_use_by_refname("b3_v2");
        level_trigger_set_script_use_by_refname("ab1_v2");
        level_trigger_set_script_use_by_refname("ab2_v2");
        level_trigger_set_script_use_by_refname("ab3_v2");
        level_trigger_set_script_use_by_refname("ab4_v2");
        level_trigger_set_exit_by_refname("d1_v3");
        level_trigger_set_exit_by_refname("b1_v3");
        level_trigger_set_exit_by_refname("b2_v3");
        level_trigger_set_exit_by_refname("b3_v3");
        level_trigger_set_exit_by_refname("ab1_v3");
        level_trigger_set_exit_by_refname("ab2_v3");
        level_trigger_set_exit_by_refname("ab3_v3");
        level_trigger_set_exit_by_refname("ab4_v3");
        return;
    }
    if (game_stage_get() >= 4) {
        level_trigger_set_script_use_by_refname("d1");
        level_trigger_set_script_use_by_refname("b1");
        level_trigger_set_script_use_by_refname("b2");
        level_trigger_set_script_use_by_refname("b3");
        level_trigger_set_script_use_by_refname("ab1");
        level_trigger_set_script_use_by_refname("ab2");
        level_trigger_set_script_use_by_refname("ab3");
        level_trigger_set_script_use_by_refname("ab4");
        level_trigger_set_exit_by_refname("d1_v2");
        level_trigger_set_exit_by_refname("b1_v2");
        level_trigger_set_exit_by_refname("b2_v2");
        level_trigger_set_exit_by_refname("b3_v2");
        level_trigger_set_exit_by_refname("ab1_v2");
        level_trigger_set_exit_by_refname("ab2_v2");
        level_trigger_set_exit_by_refname("ab3_v2");
        level_trigger_set_exit_by_refname("ab4_v2");
        level_trigger_set_script_use_by_refname("d1_v3");
        level_trigger_set_script_use_by_refname("b1_v3");
        level_trigger_set_script_use_by_refname("b2_v3");
        level_trigger_set_script_use_by_refname("b3_v3");
        level_trigger_set_script_use_by_refname("ab1_v3");
        level_trigger_set_script_use_by_refname("ab2_v3");
        level_trigger_set_script_use_by_refname("ab3_v3");
        level_trigger_set_script_use_by_refname("ab4_v3");
    } else {
        level_trigger_set_exit_by_refname("d1");
        level_trigger_set_exit_by_refname("b1");
        level_trigger_set_exit_by_refname("b2");
        level_trigger_set_exit_by_refname("b3");
        level_trigger_set_exit_by_refname("ab1");
        level_trigger_set_exit_by_refname("ab2");
        level_trigger_set_exit_by_refname("ab3");
        level_trigger_set_exit_by_refname("ab4");
        level_trigger_set_script_use_by_refname("d1_v2");
        level_trigger_set_script_use_by_refname("b1_v2");
        level_trigger_set_script_use_by_refname("b2_v2");
        level_trigger_set_script_use_by_refname("b3_v2");
        level_trigger_set_script_use_by_refname("ab1_v2");
        level_trigger_set_script_use_by_refname("ab2_v2");
        level_trigger_set_script_use_by_refname("ab3_v2");
        level_trigger_set_script_use_by_refname("ab4_v2");
        level_trigger_set_script_use_by_refname("d1_v3");
        level_trigger_set_script_use_by_refname("b1_v3");
        level_trigger_set_script_use_by_refname("b2_v3");
        level_trigger_set_script_use_by_refname("b3_v3");
        level_trigger_set_script_use_by_refname("ab1_v3");
        level_trigger_set_script_use_by_refname("ab2_v3");
        level_trigger_set_script_use_by_refname("ab3_v3");
        level_trigger_set_script_use_by_refname("ab4_v3");
    }
}

// 0x4efdb0  Confidence: exact (ends in a tail jump to lsf_lenele1aa_jekhar_talk). level_script_table frame:
// the first visit only (the record name, not the _v2 / _v3 variants).
void ls_lenele1aa_frame()
{
    if (!_stricmp(level_script_name, "lenele1aa")) {
        lsf_lenele_flag_cart_played();
        lsf_lenele1aa_jekhar_talk();
    }
}

// 0x4efde0  Confidence: exact. $apc073 within 3: the apprentice and the cartwright go; the camera call
// at $camera004 is an empty stub.
void lsf_lenele_flag_cart_played()
{
    vector3 pos;
    matrix orient;

    if (!savegame_state.get_flag(cur_level_index, "lenele_flag_cart_played")
        && script_player_near_navpoint("$apc073", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele_flag_cart_played", true);
        script_npc_send_go("Apprentice#C");
        script_npc_send_go("Cartwright");
        navpoint_get_position("$camera004", &pos, &orient);
        game_camera.cut_to_view_stub(&pos, &orient, 17.0f, 2.0f, 2.0f);
    }
}

// 0x4efe80  Confidence: exact. Jekhar talks at $apc012 (within 5) when no conversation runs.
void lsf_lenele1aa_jekhar_talk()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele1aa_jekhar_talk") && conversation_active.is_idle()
        && script_player_near_navpoint("$apc012", 5.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele1aa_jekhar_talk", true);
        script_npc_talk("Jekhar#$npc012");
    }
}

// 0x4efef0  Confidence: exact. level_script_table use.
char ls_lenele1aa_use(const char *name)
{
    if (!_stricmp(name, "sewer"))
        script_party_member_say("Joseph", XSTR("This grate is locked. I can't get in.", 500), 5.0f);
    return 0;
}

// 0x4eff30  Confidence: exact. level_script_table action: Krys's two gargoyle blood topics by the count
// (six or more, some, none).
void ls_lenele1aa_action(dialogue_action *action)
{
    int count;

    if (!_stricmp(action->name, "CheckBlood")) {
        count = party_count_item_by_name("Gargoyle Blood");
        if (count >= 6) {
            conversation_db.set_topic_available("Krys the Lantern Maker", "\"I have six drams of gargoyle blood.\"", true);
            conversation_db.set_topic_available("Krys the Lantern Maker", "\"I have some gargoyle blood.\"", false);
        } else if (count > 0) {
            conversation_db.set_topic_available("Krys the Lantern Maker", "\"I have six drams of gargoyle blood.\"", false);
            conversation_db.set_topic_available("Krys the Lantern Maker", "\"I have some gargoyle blood.\"", true);
        } else {
            conversation_db.set_topic_available("Krys the Lantern Maker", "\"I have six drams of gargoyle blood.\"", false);
            conversation_db.set_topic_available("Krys the Lantern Maker", "\"I have some gargoyle blood.\"", false);
        }
    }
}
