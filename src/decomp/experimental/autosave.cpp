// [Experimental] Autosave (docs/experimental-build.md). Saves the game into its own rotating files
// autosave1.sav .. autosave<AutosaveSlots>.sav, never into the player's summoner<n>.sav or quicksave.sav:
//   - every AutosaveMinutes of game time in play (the clock runs only in gameplay, unpaused, no menu, no
//     cutscene, no conversation), and
//   - once after a level change (AutosaveOnLevelChange), when the new level has loaded and the game has
//     been in play for a moment (not during the load, not after loading a savegame).
// A save is made only when the game itself would allow one (savegame_location_allowed == 0, the test of
// the Save screen, F3 and Ctrl+S) and nothing else is going on (see autosave_blocked); otherwise it waits
// and retries every frame, logging the reason once. The Load screen lists every *.sav of the savegame
// folder (saveload_scan_saves), newest first, so the autosaves appear there with the level, the game time
// and the description "Autosave <n> - <level> - <h:mm am>".
// The save is the one quicksave does: saveload_capture_thumbnail, conversation_db.save_state,
// savegame_state.write. It runs at exp_frame_end, after the frame was drawn: the thumbnail render
// draws into the top-left 201x151 of the back buffer, so that frame is not shown (GF_NO_FLIP; FrameEnd
// clears the flag) and the next frame's gr_clear removes it.
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD
#include "vsdk/vsdk.h"
#include "vsdk/vfile/file.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/timestamp.h"
#include "summoner/summoner.h"
#include "summoner/level_info.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_saveload.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define AUTOSAVE_MAX_SLOTS      10      // autosave1.sav .. autosave10.sav
#define AUTOSAVE_MAX_SAVES      32      // the Load screen lists 32 files (quicksave refuses a 33rd)
#define AUTOSAVE_SETTLE_SECONDS 2.0f    // in-play time after a level change before its autosave

static bool  autosave_on;               // [Experimental] Autosave
static float autosave_interval;         // AutosaveMinutes * 60, 0 = no timed saves
static bool  autosave_on_level;         // AutosaveOnLevelChange
static int   autosave_slots;            // AutosaveSlots

static float autosave_play_time;        // seconds in play since the last autosave (or load)
static bool  autosave_level_pending;    // a level change waits for its autosave
static float autosave_level_settle;     // seconds in play since that level change
static const char *autosave_last_skip;  // the last skip reason logged (logged again only when it changes)

static bool autosave_multiplayer()
{
    return !(game_flags & GF_SINGLE_PLAYER) || (game_flags & GF_MULTIPLAYER);
}

// Whether the game is in play this frame: the clock of the timer and of the level settle time.
static bool autosave_in_play()
{
    if (gameseq_get_state() != 4 || gameseq_state_pending)
        return false;
    if (game_is_paused() || game_frozen)
        return false;
    if (game_flags & (GF_CUTSCENE | GF_CONVERSATION | GF_NO_INPUT | GF_PARTY_DEAD))
        return false;
    if (interface_cur_screen != 0)
        return false;
    return true;
}

// NULL when a save may be made now, else why not. The game's own test first, then everything the
// Save screen and quicksave get for free by being opened from the HUD (no menu, no cutscene, ...).
static const char *autosave_blocked()
{
    if (autosave_multiplayer())
        return "multiplayer";
    if (gameseq_get_state() != 4)
        return "not in gameplay";
    if (gameseq_state_pending)
        return "state change pending";
    if (savegame_state.pending_load_length())
        return "savegame load pending";
    if (!cur_character)
        return "no character";
    if (game_flags & GF_PARTY_DEAD)
        return "party dead";
    if (game_flags & (GF_CUTSCENE | GF_ESCAPE_CUTSCENE))
        return "cutscene";
    if (game_flags & GF_CONVERSATION)
        return "conversation";
    if (game_flags & (GF_NO_INPUT | GF_NO_RENDER | GF_NO_CLEAR | GF_NO_FLIP | GF_SCREENSHOT))
        return "scripted sequence";
    if (game_is_paused() || game_frozen)
        return "paused";
    if (interface_cur_screen != 0)
        return "menu open";
    if (ui_message_box_is_active())
        return "message box";
    if (ui_input_locked)
        return "HUD hidden";
    if (ui_using_list_item || ui_using_equipped_item)
        return "item in use";
    switch (savegame_location_allowed()) {
    case 0:
        break;
    case 1:
        return "combat or scripted event (savegame_location_allowed 1)";
    case 2:
        return "not in this area (savegame_location_allowed 2)";
    case 3:
        return "summon emerging (savegame_location_allowed 3)";
    default:
        return "savegame_location_allowed refused";
    }
    return NULL;
}

// The number of *.sav files (quicksave's count) and the autosave slot to write: the first missing
// autosave<n>.sav, else the one with the oldest file time. *exists is set when that file is there.
static int autosave_pick_slot(int *count, bool *exists)
{
    file_find_list list;
    file_find_node *node;
    time_t times[AUTOSAVE_MAX_SLOTS + 1];
    bool found[AUTOSAVE_MAX_SLOTS + 1];
    int i, slot;

    for (i = 0; i <= AUTOSAVE_MAX_SLOTS; i++) {
        found[i] = false;
        times[i] = 0;
    }
    *count = 0;
    file_find_files(&list, path_savegame, "*.sav", 4, 0);   // 4: keep the extension, no sorting
    for (node = list.next; node != (file_find_node *)&list; node = node->next) {
        int n = 0;
        char tail[8];

        (*count)++;
        // Exactly "autosave<n>.sav" (any case).
        if (_strnicmp(node->name, "autosave", 8) == 0 && sscanf(node->name + 8, "%d%7s", &n, tail) == 2
            && _stricmp(tail, ".sav") == 0 && n >= 1 && n <= autosave_slots) {
            found[n] = true;
            times[n] = node->time;
        }
    }
    while (list.next != (file_find_node *)&list) {
        node = list.next;
        list.next = node->next;
        delete node;
    }
    list.next = list.prev = (file_find_node *)&list;

    for (i = 1; i <= autosave_slots; i++) {
        if (!found[i]) {
            *exists = false;
            return i;
        }
    }
    slot = 1;
    for (i = 2; i <= autosave_slots; i++)
        if (times[i] < times[slot])
            slot = i;
    *exists = true;
    return slot;
}

// Writes one autosave. Returns true when the file was written.
static bool autosave_write(const char *why)
{
    char filename[32];
    char level_name[128];
    char description[256];
    char clock[16];
    int count, slot, hour;
    bool exists;
    time_t now;
    struct tm *t;

    slot = autosave_pick_slot(&count, &exists);
    if (!exists && count >= AUTOSAVE_MAX_SAVES) {
        exp_log("autosave: skipped (%s): %d savegames, the Load screen shows no more", why, count);
        return false;
    }
    sprintf(filename, "autosave%d.sav", slot);

    level_format_display_name(level_name, cur_level_index);
    time(&now);
    t = localtime(&now);
    clock[0] = 0;
    if (t) {
        hour = t->tm_hour % 12;
        if (hour == 0)
            hour = 12;
        sprintf(clock, "%d:%02d %s", hour, t->tm_min, t->tm_hour < 12 ? "am" : "pm");
    }
    _snprintf(description, sizeof(description) - 1, "Autosave %d - %s - %s", slot, level_name, clock);
    description[sizeof(description) - 1] = 0;

    // As quicksave (0x40e7c0): the thumbnail, the conversation state, then the file.
    saveload_capture_thumbnail();
    game_flags |= GF_NO_FLIP;           // this frame has the thumbnail in its corner: do not show it
    conversation_db.save_state();
    if (!savegame_state.write(filename, 0, description)) {
        exp_log("autosave: FAILED to write %s (%s)", filename, why);
        hud_show_message("Autosave failed.");
        return false;
    }
    exp_log("autosave: wrote %s (%s, %s, level %d %s, game time %.0f s, %d savegames): \"%s\"", filename, why,
            exists ? "replaced the oldest" : "new slot", cur_level_index, level_filename, level_time,
            exists ? count : count + 1, description);
    if (!hud_message_timer.valid() || hud_message_timer.elapsed())
        hud_show_message("Autosave");
    return true;
}

void exp_autosave_init()
{
    int minutes;

    autosave_on = exp_ini_int("Experimental", "Autosave", 0) != 0;
    minutes = exp_ini_int("Experimental", "AutosaveMinutes", 5);
    if (minutes < 0)
        minutes = 0;
    if (minutes > 600)
        minutes = 600;
    autosave_interval = minutes * 60.0f;
    autosave_on_level = exp_ini_int("Experimental", "AutosaveOnLevelChange", 1) != 0;
    autosave_slots = exp_ini_int("Experimental", "AutosaveSlots", 3);
    if (autosave_slots < 1)
        autosave_slots = 1;
    if (autosave_slots > AUTOSAVE_MAX_SLOTS)
        autosave_slots = AUTOSAVE_MAX_SLOTS;

    autosave_play_time = 0.0f;
    autosave_level_pending = false;
    autosave_level_settle = 0.0f;
    autosave_last_skip = NULL;
    exp_log("autosave: %s, every %d min of play (0 = never), on level change %d, %d slots",
            autosave_on ? "on" : "off", minutes, autosave_on_level ? 1 : 0, autosave_slots);
}

void exp_autosave_frame()
{
    const char *why;
    const char *blocked;

    if (!autosave_on)
        return;

    if (autosave_in_play()) {
        autosave_play_time += reported_frametime;
        if (autosave_level_pending)
            autosave_level_settle += reported_frametime;
    }

    if (autosave_level_pending && autosave_level_settle >= AUTOSAVE_SETTLE_SECONDS)
        why = "level change";
    else if (autosave_interval > 0.0f && autosave_play_time >= autosave_interval)
        why = "timer";
    else
        return;

    blocked = autosave_blocked();
    if (blocked) {
        if (blocked != autosave_last_skip)
            exp_log("autosave: waiting (%s): %s", why, blocked);
        autosave_last_skip = blocked;
        return;
    }
    autosave_last_skip = NULL;

    // Written or not, start over: a failed write is not retried every frame.
    autosave_write(why);
    autosave_play_time = 0.0f;
    autosave_level_pending = false;
    autosave_level_settle = 0.0f;
}

// level_load (0x438570), from state_game_enter: the level files are loaded; a pending savegame is read
// after this (savegame::load_pending), then the game runs.
void exp_autosave_level_loaded(bool ok)
{
    if (!autosave_on)
        return;
    autosave_last_skip = NULL;
    autosave_level_settle = 0.0f;
    autosave_level_pending = false;
    if (!ok)
        return;
    if (savegame_state.pending_load_length()) {
        // A savegame is being loaded: the player's own save point, so no autosave, and the timer starts over.
        autosave_play_time = 0.0f;
        exp_log("autosave: level %d (%s) loaded from a savegame: no autosave, timer reset", cur_level_index,
                level_filename);
        return;
    }
    if (autosave_multiplayer() || !autosave_on_level)
        return;
    autosave_level_pending = true;
    exp_log("autosave: level %d (%s) loaded: autosave after %.0f s in play", cur_level_index, level_filename,
            AUTOSAVE_SETTLE_SECONDS);
}
bool exp_autosave_enabled()
{
    return autosave_on;
}

#endif
