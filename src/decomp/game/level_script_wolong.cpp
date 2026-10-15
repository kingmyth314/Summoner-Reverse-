// level_script_wolong.cpp - Sum.exe v1.40, code 0x4fe8f0-0x4feb4d (levelscripts\Scripts\
// level_script_wolong.cpp; strings.tbl id 560 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "Wolong" (the village, stage 10: Rosalind rejoins, Sizhe's four items, the
// draught that opens the way to the caverns). No static initialiser.
// Data: .bss 0x257ed7c. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_WOLONG2      0x1c                      // "got_ring_of_wind" (0x4fe913)

// ---- .bss (0x257ed7c) ----------------------------------------------------------------------------------
bool ls_wolong_place_pending;                        // 0x257ed7c

// 0x4fe8f0  Confidence: exact. level_script_table init (stage 10). Jekhar is out of the party until the
// Ring of Wind (Wolong2); Rosalind until found_rosalind2 (then her NPC copy goes); the caverns exit opens
// with got_draught.
void ls_wolong_init()
{
    level_music = music_find_by_name("wolong");
    game_stage_set(0xa);
    if (!savegame_state.get_flag(LEVEL_WOLONG2, "got_ring_of_wind"))
        party_remove_member("Jekhar");
    if (!savegame_state.get_flag(cur_level_index, "found_rosalind2"))
        party_remove_member("Rosalind");
    else
        script_npc_set_disabled("Rosalind#npc", true);
    if (savegame_state.get_flag(cur_level_index, "got_draught"))
        level_trigger_set_exit("wolongcaverns", NULL);
    else
        level_trigger_set_script_use("wolongcaverns", NULL);
}

// 0x4fe9a0  Confidence: exact. level_script_table frame. Nar-Wolong once; after AddRosalind (and no
// conversation) Rosalind joins and is placed at $npc003.
void ls_wolong_frame()
{
    Player *p;
    Character *c;

    if (!savegame_state.get_flag(cur_level_index, "wolong_intro")) {
        savegame_state.set_flag(cur_level_index, "wolong_intro", true);
        script_play_cutscene("Nar-Wolong", NULL);
    }
    if (ls_wolong_place_pending && !(game_flags & GF_CONVERSATION)) {
        script_npc_set_disabled("Rosalind#npc", true);
        p = party_add_member("Rosalind");
        c = living_from_handle(p->handle);
        level_entry_point = 3;
        c->place_at_marker(navpoint_find("$npc003"));
        ls_wolong_place_pending = false;
    }
}

// 0x4fea40  Confidence: exact. level_script_table use.
char ls_wolong_use(const char *name)
{
    if (!_stricmp(name, "wolongcaverns"))
        script_party_member_say("Joseph", XSTR("I'm not ready to go down there yet.", 560), 5.0f);
    return 0;
}

// 0x4fea80  Confidence: exact. level_script_table action. GotDraught only opens the exit; the flag
// got_draught that ls_wolong_init tests is set by the same Wolong_dlg.tbl topic (+Event: "got_draught").
void ls_wolong_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "AddRosalind")) {
        ls_wolong_place_pending = true;
    } else if (!_stricmp(action->name, "CheckItems")) {
        if (party_count_item_by_name("Onyx Carp") && party_count_item_by_name("Gulan's Knife")
            && party_count_item_by_name("Stick of Charcoal") && party_count_item_by_name("Minhe's Bowl"))
            conversation_db.set_topic_available("Sizhe", "\"I brought the things you asked for.\"", true);
        else
            conversation_db.set_topic_available("Sizhe", "\"I brought the things you asked for.\"", false);
    } else if (!_stricmp(action->name, "GotDraught")) {
        level_trigger_set_exit("wolongcaverns", NULL);
    }
}
