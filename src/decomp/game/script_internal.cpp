// script_internal.cpp - Sum.exe v1.40, code 0x4db340-0x4de0a3 (levelscripts\script_internal.cpp: the
// #Navpoints / #Objects parsers of the level and cutscene scripts, the script actions and the script
// entry state machine)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit path from the assert literal 0x5a0670 (DebugError line 899 in script_parse_navpoint).
// Data: .data 0x5a045c-0x5a08a7 (script_action_names, script_navpoint_type_names, their strings in reverse
// order, then the unit's literals; "wander", "follow", "level", "camera", "$npc", "$player", "$Name:",
// "$Type:", "+Plane:", "$Character:", "+Prop:", "run", "#End", "" are pooled copies of earlier units),
// .bss 0x2546760-0x2546823 (cutscene_saved_equipment; 0x2546828 starts the script editor's).
// Float constants: 0.5 (0x57e6b0), 0 (0x57eb84), 2.0 (0x580200), 0.1 (0x580214), 20.0 (0x581960), all
// __real@ COMDATs of other units.
// str_copy_append_32 (0x4db250) and filename_has_cscript_suffix (0x4db2c0) lie between conversationctl.cpp
// and this unit and have no module tag; they are written here as this file's first functions (D5601).

#include <stdio.h>
#include <string.h>
#include "vsdk/os/console.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/parse/parse.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/ai/boss_ai.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/script_internal.h"

// Original __FILE__ of this unit (literal 0x5a0670).
#define SCRIPT_INTERNAL_CPP "D:\\projects\\Summoner\\pccode\\levelscripts\\script_internal.cpp"

// ---- .data (0x5a045c-0x5a04ef) ------------------------------------------------------------------------
const char *script_action_names[NUM_SCRIPT_ACTIONS] = {      // 0x5a045c
    "walk to", "run to", "move through", "wait at", "wander", "wander for", "do state", "play action",
    "play .mvf", "play effect", "turn hostile", "wait for go", "say", "die", "label", "goto", "no block",
    "trigger", "trigger nav", "trigger visit", "teleport to", "pass go", "show/hide", "synch",
    "synch teleport", "follow", "combat mode", "mount anim", "dismount anim", "replace weapon",
    "guard spline", "replace armor", "one hand",
};
const char *script_navpoint_type_names[4] = { "level", "cutscene", "camera", "ambient sound" };   // 0x5a04e0

// ---- .bss (0x2546760-0x2546823) ----------------------------------------------------------------------
cutscene_saved_item cutscene_saved_equipment[MAX_CUTSCENE_SAVED_ITEMS];   // 0x2546760
int cutscene_saved_equipment_count;                                      // 0x2546820

// ---- the two file name helpers before the parsers (0x4db250-0x4db33e, untagged; D5601) ---------------
// 0x4db250  Confidence: exact. base cut to 32 - strlen(suffix) characters, then suffix (strcpy expanded).
// strncpy does not terminate; the strcpy does.
void str_copy_append_32(char *out, const char *base, const char *suffix)
{
    size_t n = strlen(base);
    size_t room = 32 - strlen(suffix);

    if (room < n)
        n = room;
    strncpy(out, base, n);
    strcpy(out + n, suffix);
}

// 0x4db2c0  Confidence: exact (the suffix length is computed before the NULL test). True when the name
// is longer than "_cscript.tbl" and ends with it (case-sensitive).
bool filename_has_cscript_suffix(const char *name)
{
    int suffix_len = strlen("_cscript.tbl");
    int len;

    if (name == NULL)
        return false;
    len = strlen(name);
    if (len < suffix_len + 1)
        return false;
    if (strcmp(name + len - suffix_len, "_cscript.tbl") == 0)
        return true;
    return false;
}

// 0x4db340  Confidence: exact (VC6 byte match) (the copy of the orientation as one matrix assignment). A living entity
// that already started at this navpoint (+No create re-runs, level_script_restart) is moved back onto it
// instead of creating a second one.
int script_object_spawn_at_navpoint(int char_info, int type, navpoint *np, int level)
{
    vector3 pos;
    matrix orient;
    Character *c;

    navpoint_get_position(np->name, &pos, &orient);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->start_navpoint == np) {
            c->pos = pos;
            c->orient = orient;
            return c->handle;
        }
    }

    c = living_entity_create(type, char_info, &pos, &orient, true, np->layer, level, 0, false);
    c->start_navpoint = np;
    return c->handle;
}

// 0x4db420  Confidence: exact (D12109: the ray start and direction as vector copies, the hit record cleared
// just before the raycast, the level object through a local; jump table 0x4db6ec over the type). One
// $Name: record of a cutscene script's #Navpoints. Level and cutscene navpoints are dropped onto the
// floor 2 units above them (a 1000-unit ray down, lobj flag 0x40 required): the height above it is kept,
// a navpoint below the floor is moved onto it. A miss is a fatal DebugError.
void script_parse_navpoint(navpoint *np, TextFile *tf)
{
    float p, b, h;
    vector3 hit_pos;
    lobj_ray ray;
    lobj_hit hit;
    matrix orient;
    char buf[256];
    lobj *l;

    tf->Expect("$Name:");
    tf->ParseString(np->name, 0xf, '"', '"');
    if (tf->Accept("$Type:"))
        np->type = tf->ParseTypeString(script_navpoint_type_names, 4);
    else if (_strnicmp(np->name, "$npc", 4) == 0 || _strnicmp(np->name, "$player", 7) == 0)
        np->type = NAVPOINT_TYPE_LEVEL;
    else if (_strnicmp(np->name, "$c_npc", 6) == 0)
        np->type = NAVPOINT_TYPE_CUTSCENE;
    else if (_strnicmp(np->name, "$camera", 7) == 0)
        np->type = NAVPOINT_TYPE_CAMERA;
    else
        np->type = NAVPOINT_TYPE_LEVEL;

    np->layer = -1;
    if (tf->Accept("+Plane:"))
        np->layer = tf->ParseInt();
    tf->Expect("$Position:");
    tf->ParseVector(&np->pos);
    tf->Expect("$Orientation:");
    tf->ParseMatrix(&orient);
    orient.extract_angles(&p, &b, &h);
    np->heading = h;
    np->ground_height = 0;

    switch (np->type) {
    case NAVPOINT_TYPE_LEVEL:
    case NAVPOINT_TYPE_CUTSCENE:
        ray.start = np->pos;
        ray.start.y += 2.0f;
        ray.dir = vector3(0.0f, -1000.0f, 0.0f);
        ray._unk18 = 0.0f;
        ray.flags = 0;
        ray.require_flags = 0x40;
        ray.exclude_flags = 0;
        memset((void *)&hit, 0, sizeof(hit));
        lobj_raycast_all(&ray, &hit);
        if (hit.num_hits == 0) {
            sprintf(buf, "Couldn't collide navpoint %s with the ground -- chances are that\n"
                         "it is below ground or above a mesh not in a $layer set.", np->name);
            VSDK_ERROR(SCRIPT_INTERNAL_CPP, 899, buf);
        }
        hit_pos = ray.start + ray.dir * hit.t;
        np->ground_height = np->pos.y - hit_pos.y;
        l = lobj_from_index(hit.face);
        np->layer = l->pick_layer(&hit_pos, 1);
        if (np->ground_height < 0.0f) {
            np->pos = hit_pos;
            np->ground_height = 0.0f;
        }
        break;
    case NAVPOINT_TYPE_CAMERA:
    case NAVPOINT_TYPE_AMBIENT:
        np->layer = -1;
        break;
    }
    np->level_item = -1;
}

// 0x4db700  Confidence: exact (as script_parse_navpoint, D12109; the name buffer is 80 bytes, the binary's
// frame, D12101). One $Name:
// record of a level script's #Navpoints: an existing level navpoint of that name is updated (the last
// match: the search does not stop), otherwise one is appended. There is no miss check: without a hit
// the zeroed record gives the start point (2 units up) and level object 0.
void script_parse_navpoints(TextFile *tf)
{
    float p, b, h;
    vector3 hit_pos;
    lobj_ray ray;
    lobj_hit hit;
    matrix orient;
    char name[80];
    navpoint *np = NULL;
    int i;
    lobj *l;

    tf->Expect("$Name:");
    tf->ParseString(name, 0xf, '"', '"');
    for (i = 0; i < navpoint_count; i++) {
        if (_stricmp(navpoints[i].name, name) == 0)
            np = &navpoints[i];
    }
    if (np == NULL) {
        np = &navpoints[navpoint_count++];
        strcpy(np->name, name);
    }

    if (tf->Accept("$Type:"))
        np->type = tf->ParseTypeString(script_navpoint_type_names, 4);
    else if (_strnicmp(np->name, "$npc", 4) == 0 || _strnicmp(np->name, "$player", 7) == 0)
        np->type = NAVPOINT_TYPE_LEVEL;
    else if (_strnicmp(np->name, "$c_npc", 6) == 0)
        np->type = NAVPOINT_TYPE_CUTSCENE;
    else if (_strnicmp(np->name, "$camera", 7) == 0)
        np->type = NAVPOINT_TYPE_CAMERA;
    else
        np->type = NAVPOINT_TYPE_LEVEL;

    np->layer = -1;
    if (tf->Accept("+Plane:"))
        np->layer = tf->ParseInt();
    tf->Expect("$Position:");
    tf->ParseVector(&np->pos);
    tf->Expect("$Orientation:");
    tf->ParseMatrix(&orient);
    orient.extract_angles(&p, &b, &h);
    np->heading = h;
    np->ground_height = 0;

    switch (np->type) {
    case NAVPOINT_TYPE_LEVEL:
    case NAVPOINT_TYPE_CUTSCENE:
        ray.start = np->pos;
        ray.start.y += 2.0f;
        ray.flags = 0;
        ray.exclude_flags = 0;
        ray.dir = vector3(0.0f, -1000.0f, 0.0f);
        ray._unk18 = 0.0f;
        ray.require_flags = 0x40;
        memset((void *)&hit, 0, sizeof(hit));
        lobj_raycast_all(&ray, &hit);
        hit_pos = ray.start + ray.dir * hit.t;
        np->ground_height = np->pos.y - hit_pos.y;
        l = lobj_from_index(hit.face);
        np->layer = l->pick_layer(&hit_pos, 1);
        if (np->ground_height < 0.0f) {
            np->pos = hit_pos;
            np->ground_height = 0.0f;
        }
        break;
    case NAVPOINT_TYPE_CAMERA:
    case NAVPOINT_TYPE_AMBIENT:
        np->layer = -1;
        break;
    }
    np->level_item = -1;
}

// 0x4dba30  Confidence: exact (VC6 byte match, D22701: the start navpoint in a block-local np before the spawn
// call, the trigger flag as `if (!get_flag) set_ai_active(0); else set_ai_active(1);`, Play action's move id
// through n and an if / else for the anim; was likely: local layout and the order of the two +Hidden / +Trigger: tests; jump
// table 0x4dc448 over the 33 action types). One $Name: record of #Objects. "+No create" binds the
// script to a living entity or party member that already has the name (a dead party member is
// resurrected with 1 HP); otherwise the character is created at its start navpoint. Each "+Action:"
// adds one action; the first one allocates the script object (level: script_objects, end mode 0;
// cutscene: cutscene_script_objects, end mode 2).
void script_parse_objects(TextFile *tf, bool level)
{
    char navname[16];
    char name[32];
    char propname[32];
    char charname[32];
    char text[256];
    char trigger[256];
    char itemname[256];
    Character *c;
    script_object *so;
    script_action *a;
    move_info *m;
    int trigger_level = -1;
    bool hidden = false;
    bool level_set;
    bool miniboss;
    int ci, type, team, char_level, prop, n;

    tf->Expect("$Name:");
    tf->ParseString(name, 0x20, '"', '"');

    if (tf->Accept("+No create")) {
        tf->Expect("$Start position:");
        tf->ParseString(navname, 0xf, '"', '"');
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (_stricmp(c->get_name(), name) == 0)
                break;
        }
        if (c == &living_list || (c->type == 7 && c->hp <= 0.0f)) {
            Player *p;
            for (p = party_list.next; p != &party_list; p = p->next) {
                c = living_from_handle(p->handle);
                if (_stricmp(c->get_name(), name) == 0) {
                    if (c->hp <= 0.0f) {
                        c->resurrect();
                        c->hp = 1.0f;
                    }
                    break;
                }
            }
            if (p == &party_list) {
                if (!tf->SkipTo("$Name", "#End"))
                    tf->SkipTo("#End", NULL);
                return;
            }
        }
    } else {
        tf->Expect("$Character:");
        tf->ParseString(charname, 0x20, '"', '"');
        ci = charinfo_find_by_name(charname);
        if (tf->Accept("+Hidden"))
            hidden = true;
        if (tf->Accept("+Trigger:")) {
            tf->ParseString(text, 0xff, '"', '"');
            trigger_level = level_script_find(text);
            tf->ParseString(trigger, 0xff, '"', '"');
        } else {
            trigger_level = -1;
            strcpy(trigger, "");
        }
        if (tf->Accept("+Monster"))
            type = 5;
        else
            type = tf->Accept("+Boss") ? 9 : 4;
        miniboss = false;
        if (tf->Accept("+Miniboss"))
            miniboss = true;
        if (tf->Accept("+Hidden"))
            hidden = true;
        if (tf->Accept("+Trigger:")) {
            tf->ParseString(text, 0xff, '"', '"');
            trigger_level = level_script_find(text);
            tf->ParseString(trigger, 0xff, '"', '"');
        }
        if (species_is_boss(char_infos[ci].species))
            type = 9;
        team = char_infos[ci].team;
        if (tf->Accept("+Team:"))
            team = tf->ParseTypeString(team_names, 4);
        else if ((game_flags & 2) && type == 5)
            team = 2;
        char_level = 0;
        level_set = false;
        if (tf->Accept("+Level:")) {
            char_level = tf->ParseInt();
            if (char_level >= 0)
                level_set = true;
        }
        prop = -1;
        if (tf->Accept("+Prop:")) {
            tf->ParseString(propname, 0x20, '"', '"');
            prop = item_type_find(propname);
            if (prop < 0)
                sprintf(string_buffer, "Unable to find prop %s for character %s\n", propname, name);
        }
        tf->Expect("$Start position:");
        tf->ParseString(navname, 0xf, '"', '"');

        {   // D22701: navpoint_find is called before the spawn's arguments are pushed
            navpoint *np = navpoint_find(navname);
            c = living_from_handle(script_object_spawn_at_navpoint(ci, type, np, char_level));
        }
        if (level_set)
            c->flags |= 0x400000;
        if (miniboss)
            c->flags |= 0x40;
        c->set_team(team);
        c->set_name(name);

        if (species_is_hero(c->info->species)) {
            Player *p;
            for (p = party_list.next; p != &party_list; p = p->next) {
                Character *t = living_from_handle(p->handle);
                if (t != NULL && t->info->species == c->info->species) {
                    script_object_copy_from_template(t, c);
                    break;
                }
            }
            for (p = player_list_other.next; p != &player_list_other; p = p->next) {
                Character *t = living_from_handle(p->handle);
                if (t != NULL && t->info->species == c->info->species) {
                    script_object_copy_from_template(t, c);
                    break;
                }
            }
        } else if (prop != -1) {
            item_info *old1, *old2;
            character_equip_item(&old1, &old2, c, 5, item_create_weapon(prop));
            if (old1)
                item_free(old1);
            if (old2)
                item_free(old2);
        }
    }

    so = NULL;
    while (tf->Accept("+Action:")) {
        if (so == NULL) {
            if (level) {
                c->script_object = script_object_count;
                so = &script_objects[script_object_count];
                so->end_mode = 0;
                script_object_count++;
            } else {
                c->cutscene_object = cutscene_script_object_count;
                printf("Assigning %s curscent script %d\n", c->name, cutscene_script_object_count);
                so = &cutscene_script_objects[cutscene_script_object_count];
                so->end_mode = 2;
                cutscene_script_object_count++;
            }
            so->num_actions = 0;
            so->cur_action = -1;
            so->timer.invalidate();
            so->handle = c->handle;
            so->path = NULL;
            so->trigger_level = trigger_level;
            strcpy(so->trigger, trigger);
            so->hidden = hidden;
            if (hidden)
                c->set_ai_active(0);
            if (trigger_level != -1) {   // D22701: two calls (push eax / push 1, one shared call)
                if (!savegame_state.get_flag(trigger_level, trigger))
                    c->set_ai_active(0);
                else
                    c->set_ai_active(1);
            }
        }

        a = &so->actions[so->num_actions];
        a->type = -1;
        a->name = NULL;
        a->arg0c = 0xcdcdcd;
        so->num_actions++;
        a->type = tf->ParseTypeString(script_action_names, NUM_SCRIPT_ACTIONS);

        switch (a->type) {
        case SCRIPT_ACTION_WALK_TO:
        case SCRIPT_ACTION_RUN_TO:
        case SCRIPT_ACTION_MOVE_THROUGH:
        case SCRIPT_ACTION_TELEPORT_TO:
            tf->ParseString(navname, 0xf, '"', '"');
            a->nav = navpoint_find(navname);
            break;

        case SCRIPT_ACTION_WAIT_AT:
        case SCRIPT_ACTION_TRIGGER_NAV:
            tf->ParseString(navname, 0xf, '"', '"');
            a->nav = navpoint_find(navname);
            a->arg0c = tf->ParseInt();
            break;

        case SCRIPT_ACTION_WANDER:
        case SCRIPT_ACTION_WANDER_FOR:
            tf->ParseString(navname, 0xf, '"', '"');
            a->nav = navpoint_find(navname);
            a->arg0c = tf->ParseInt();
            a->arg10 = -1;
            if (a->type == SCRIPT_ACTION_WANDER_FOR)
                a->arg10 = tf->ParseInt();
            a->flag14 = 1;
            if (tf->Peek("run")) {
                tf->Expect("run");
                a->flag14 = 0;
            }
            break;

        case SCRIPT_ACTION_DO_STATE:
            a->arg0c = tf->ParseTypeString((const char **)locomotion_anim_names, NUM_LOCO_ANIMS);
            break;

        case SCRIPT_ACTION_PLAY_ACTION:
            n = tf->ParseInt();   // D22701
            m = charinfo_find_move(c->info, 8, n, true);
            if (m != NULL)
                a->arg0c = m->anim;
            else
                a->arg0c = -1;
            break;

        case SCRIPT_ACTION_DIE:
            n = tf->ParseInt();
            if (c->type == 4)
                m = charinfo_find_move(c->info, 8, n, true);
            else
                m = charinfo_find_move(c->info, 6, 0x22, true);
            if (m != NULL) {
                a->arg0c = m->anim;
            } else {
                m = charinfo_find_move(c->info, 6, 0x22, true);
                if (m == NULL)
                    m = charinfo_find_move(c->info, 8, n, true);
                if (m != NULL)
                    a->arg0c = m->anim;
                else
                    a->arg0c = -1;
            }
            if (so->end_mode != 2)
                so->end_mode = 1;
            break;

        case SCRIPT_ACTION_WAIT_GO:
            a->arg0c = 0;
            break;

        case SCRIPT_ACTION_PLAY_MVF:
        case SCRIPT_ACTION_PLAY_EFFECT:
        case SCRIPT_ACTION_SAY:
        case SCRIPT_ACTION_PASS_GO:
        case SCRIPT_ACTION_FOLLOW:
            tf->ParseString(text, 0x100, '"', '"');
            if (level) {
                if (a->type == SCRIPT_ACTION_SAY)
                    localize_remap_charset(text, 1);
                a->text = level_script_stringpool.Add(text);
            } else {
                a->text = cutscene_script_stringpool.Add(text);
            }
            if (a->type == SCRIPT_ACTION_SAY)
                a->arg0c = tf->ParseInt();
            else if (a->type == SCRIPT_ACTION_PLAY_MVF)
                a->arg0c = charinfo_load_script_anim(c->info, a->text, cutscene_path_id);
            else if (a->type == SCRIPT_ACTION_FOLLOW)
                a->arg0c = tf->ParseInt();
            break;

        case SCRIPT_ACTION_TURN_HOSTILE:
        case SCRIPT_ACTION_LABEL:
        case SCRIPT_ACTION_GOTO:
        case SCRIPT_ACTION_NO_BLOCK:
        case SCRIPT_ACTION_TRIGGER:
        case SCRIPT_ACTION_TRIGGER_VISIT:
        case SCRIPT_ACTION_SHOW_HIDE:
        case SCRIPT_ACTION_SYNCH:
        case SCRIPT_ACTION_SYNCH_TELEPORT:
        case SCRIPT_ACTION_COMBAT_MODE:
        case SCRIPT_ACTION_GUARD_SPLINE:
        case SCRIPT_ACTION_ONE_HAND:
            a->arg0c = tf->ParseInt();
            if (a->type == SCRIPT_ACTION_SYNCH_TELEPORT) {
                tf->ParseString(navname, 0xf, '"', '"');
                a->nav = navpoint_find(navname);
            }
            break;

        case SCRIPT_ACTION_REPLACE_WEAPON:
        case SCRIPT_ACTION_REPLACE_ARMOR:
            tf->ParseString(itemname, 0x100, '"', '"');
            if (_stricmp(itemname, "(none)") == 0)
                a->arg0c = -1;
            else
                a->arg0c = item_type_find(itemname);
            break;

        case SCRIPT_ACTION_MOUNT_ANIM:
        case SCRIPT_ACTION_DISMOUNT_ANIM:
            break;
        }
    }
}

// 0x4dc4d0  Confidence: exact (VC6 byte match) (VC6 keeps the two equip outputs in the dead argument slots). A scripted
// hero copies the party (or other player) copy's flag 0x2000000 and gets new items of the same types
// in the equipment slots 0-8.
void script_object_copy_from_template(Character *tmpl, Character *c)
{
    int i;

    printf("Copying data from %s\n", tmpl->info->name);
    if (tmpl->flags & 0x2000000)
        c->flags |= 0x2000000;
    else
        c->flags &= ~0x2000000;

    for (i = 0; i < 9; i++) {
        if (tmpl->equipment[i] != NULL) {
            item_info *old1, *old2;
            character_equip_item(&old1, &old2, c, i, item_create_by_name(tmpl->equipment[i]->type->name));
            if (old1)
                item_free(old1);
            if (old2)
                item_free(old2);
        }
    }
}

// 0x4dc570  Confidence: exact. The level navpoints first, then the cutscene ones.
navpoint *navpoint_find(const char *name)
{
    int i;

    for (i = 0; i < navpoint_count; i++) {
        if (_stricmp(navpoints[i].name, name) == 0)
            return &navpoints[i];
    }
    for (i = 0; i < cutscene_navpoint_count; i++) {
        if (_stricmp(cutscene_navpoints[i].name, name) == 0)
            return &cutscene_navpoints[i];
    }
    return NULL;
}

// 0x4dc600  Confidence: exact. The orientation is rebuilt from the heading alone.
bool navpoint_get_position(const char *name, vector3 *pos, matrix *orient)
{
    int i;

    if (name == NULL)
        return false;

    for (i = 0; i < navpoint_count; i++) {
        if (_stricmp(navpoints[i].name, name) == 0) {
            *pos = navpoints[i].pos;
            orient->set_from_angles(0.0f, 0.0f, navpoints[i].heading);
            return true;
        }
    }
    for (i = 0; i < cutscene_navpoint_count; i++) {
        if (_stricmp(cutscene_navpoints[i].name, name) == 0) {
            *pos = cutscene_navpoints[i].pos;
            orient->set_from_angles(0.0f, 0.0f, cutscene_navpoints[i].heading);
            return true;
        }
    }
    return false;
}

// 0x4dc6e0  Confidence: likely (the walk to the last path node; D14205: closer, the binary keeps so in EBX; D22700:
// a `goal` pointer local for &np->pos, closer). Actions 0-2. "move through" only
// builds the path (appended to the one the previous move through left, flag 2 keeps the corner); the
// walk / run hands the whole path to a goto-navpoint goal (flag 0x20, the script's goal).
bool script_action_walk_to(Character *c, script_object *so)
{
    script_action *a = &so->actions[so->cur_action];
    navpoint *np;
    ai_path *last;
    ai_path *path;
    bool walk;
    int g;
    vector3 *goal;

    if (so->cur_action == 0 || so->actions[so->cur_action - 1].type != SCRIPT_ACTION_MOVE_THROUGH) {
        ai_path_free((ai_path *)so->path);
        so->path = NULL;
    }

    np = a->nav;
    goal = &np->pos;
    for (last = (ai_path *)so->path; last != NULL; last = last->next) {
        if (last->next == NULL)
            break;
    }
    if (last != NULL)
        path = last->next = ai_path_find(&last->pos, last->layer, goal, np->layer, c->info->size, 0.0f);
    else
        path = (ai_path *)(so->path = ai_path_find(&c->pos, c->layer, goal, np->layer, c->info->size, 0.0f));

    if (a->type == SCRIPT_ACTION_MOVE_THROUGH) {
        if (path != NULL) {
            path->flags |= 2;
            return true;
        }
        return false;
    }

    walk = false;
    if (a->type == SCRIPT_ACTION_WALK_TO)
        walk = true;
    g = c->ai.add_move_to(goal, np->layer, walk, 100);
    if (g == -1)
        return false;
    c->ai.goals[g].flags |= 0x20;
    if (so->path != NULL) {
        c->ai.goals[g].path = (ai_path *)so->path;
        so->path = NULL;
        if (so->end_mode == 2)
            c->ai.perception_timer.invalidate();
        else
            c->ai.perception_timer.SetTimeout(0);
    }
    return true;
}

// 0x4dc860  Confidence: exact (the target is the navpoint's forward vector plus the character's own
// position, as the code adds them, stored back into pos, D7700; vector3's copy constructor is called out
// of line for the by-value argument). The wait time is the action's seconds.
bool script_action_wait_at(Character *c, script_object *so)
{
    vector3 pos;
    matrix orient;
    script_action *a = &so->actions[so->cur_action];

    so->timer.SetTimeout(a->arg0c * 1000);
    navpoint_get_position(a->nav->name, &pos, &orient);
    pos = orient.fvec + c->pos;
    c->walk_to_point(pos, 100);
    return true;
}

// 0x4dc900  Confidence: exact.
bool script_action_do_state(Character *c, script_action *a)
{
    c->instance->set_state_weight(c->loco_anims[a->arg0c], 1.0f);
    return true;
}

// 0x4dc930  Confidence: exact. In a cutscene (game_flags 0x20) the character keeps flag 0x1000 while
// the move plays (script_entry_check_done clears it).
bool script_action_play_action(Character *c, script_action *a, bool hold)
{
    move_info *m;

    if (a->arg0c >= 0) {
        m = charinfo_find_move_any(c->info, a->arg0c, true);
        c->play_move((int)m, 1.0f, true, true, hold);
        c->find_ground();
        if (game_flags & 0x20)
            c->flags |= 0x1000;
        return true;
    }
    return false;
}

// 0x4dc990  Confidence: exact. Party members (type 4) play the death move first; a boss (type 9) dies
// as a monster.
bool script_action_die(Character *c, script_action *a)
{
    move_info *m;

    if (a->arg0c >= 0) {
        m = charinfo_find_move_any(c->info, a->arg0c, true);
        if (c->type == 4) {
            c->play_move((int)m, 1.0f, false, false, true);
            c->kill(m->id);
            c->flags |= 0x80;
        } else {
            if (c->type == 9)
                c->type = 5;
            c->kill(m->id);
        }
        return true;
    }
    return false;
}

// 0x4dca10  Confidence: exact. Jumps to the "label" action with the same number (the advance runs the
// action after it). Without the label the character's AI flag 0x40 is dropped (the same code as
// script_object_receive_go's).
bool script_action_goto(Character *c, script_action *a, script_object *so)
{
    int i;

    if (c != NULL) {
        for (i = 0; i < so->num_actions; i++) {
            script_action *l = &so->actions[i];
            if (l->type == SCRIPT_ACTION_LABEL && l->arg0c == a->arg0c) {
                so->cur_action = i;
                script_entry_advance(so, c);
                return true;
            }
        }
        if (c->ai.flags & 0x40) {
            c->frametime = reported_frametime;
            c->ai.flags &= ~0x40;
        }
    }
    return false;
}

// 0x4dca90  Confidence: exact (D12101: the layer read into a local before the call). Actions 4-5: a timed
// move-to goal (the int argument as its float) at the navpoint, walking unless the script said "run".
bool script_action_wander(Character *c, script_action *a, bool walk)
{
    navpoint *np = a->nav;
    vector3 pos;
    int g;

    if (np != NULL) {
        pos = np->pos;
        int layer = np->layer;
        g = c->ai.add_move_to_timed(&pos, layer, (float)a->arg0c, walk, 100);
        if (g != -1) {
            c->ai.goals[g].flags |= 0x20;
            return true;
        }
    }
    return false;
}

// 0x4dcb10  Confidence: exact. Action 25: joins the named script object's AI group (created on demand)
// and adds a follow goal on it; the seconds (-1 = no limit) go to the script timer.
bool script_action_follow(script_object *so, Character *c, const char *leader, int seconds)
{
    int i = script_object_find(leader);
    Character *l;

    if (i == -1)
        return false;
    l = living_from_handle(script_objects[i].handle);
    if (l == NULL)
        return false;
    if (l->ai.group == NULL)
        ai_group_create(l);
    ai_group_add_member(l->ai.group, c);
    if (seconds != -1)
        so->timer.SetTimeout(seconds * 1000);
    c->ai.goal_add(AI_GOAL_FOLLOW, 100, 0x21, l->handle, NULL, 0, AI_NO_SPELL, 0.0f);
    return true;
}

// 0x4dcbc0  Confidence: exact (D16501: teleports_only declared after the species test). The scan of the earlier actions: types 20, 23 and 24 keep it as a
// teleport-only prefix). A combat-mode goal, unless every earlier action was a teleport or synch, the
// AI is held (flag 0x40), the character has flag 8 or it is the boss of species 0x93; then the mode is
// switched directly (flag 1 and the prop tag).
bool script_action_combat_mode(script_action *a, script_object *so, Character *c)
{
    bool on = a->arg0c == 1;
    bool skip = false;
    int i;

    if (c->type == 9 && c->info->species == 0x93)
        skip = true;
    bool teleports_only = true;                               // declared here: BL, and on in a's slot (D16501)
    for (i = 0; i < so->cur_action; i++) {
        if (!teleports_only)
            break;
        switch (so->actions[i].type) {
        case SCRIPT_ACTION_TELEPORT_TO:
        case SCRIPT_ACTION_SYNCH:
        case SCRIPT_ACTION_SYNCH_TELEPORT:
            break;
        default:
            teleports_only = false;
            break;
        }
    }
    if (!teleports_only && !(c->ai.flags & 0x40) && !(c->flags & 8) && !skip)
        return c->ai.set_combat_mode_goal(on, 100) != -1 ? true : false;

    if (on == true) {
        c->flags |= 1;
        c->prop_tag = 0;
    } else {
        c->prop_tag = c->default_prop_tag;
        c->flags &= ~1;
    }
    return true;
}

// 0x4dccb0  Confidence: exact (VC6 byte match, D22703: the new item in a block-local `it` before the equip call
// (it moves c to ESI); was likely, D14205: the slot mapping written as an if / else if after the record's first two
// fields). Action 29 (cutscenes): equips a new weapon of the type (or empties slot 5 for "(none)"). The
// items taken off are kept in cutscene_saved_equipment for the rewind, once per character (a second
// replace frees them); an item whose ItemType slot is not 4, 5 or 9 is recorded with a stale slot. For
// "(none)" the off hand (slot 9) is saved and emptied too. The record count is not bounded (16).
bool script_action_replace_weapon(script_action *a, Character *c)
{
    item_info *old1 = NULL;
    item_info *old2 = NULL;
    bool save = true;
    int i;
    int s;

    for (i = 0; i < cutscene_saved_equipment_count; i++) {
        if (cutscene_saved_equipment[i].c == c) {
            s = cutscene_saved_equipment[i].slot;
            if (s == 5 || s == 9 || s == 6) {
                save = false;
                break;
            }
        }
    }

    if (a->arg0c >= 0) {
        item_info *it = item_create(a->arg0c);   // D22703
        character_equip_item(&old1, &old2, c, 5, it);
    } else
        old1 = character_unequip_slot(c, 5, 1);

    if (old1 != NULL) {
        if (save) {
            cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
            cutscene_saved_equipment[cutscene_saved_equipment_count].item = old1;
            s = old1->type->slot;
            if (s == 4 || s == 9)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 5;
            else if (s == 5)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 6;
            cutscene_saved_equipment_count++;
        } else {
            item_free(old1);
        }
    }
    if (old2 != NULL) {
        if (save) {
            cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
            cutscene_saved_equipment[cutscene_saved_equipment_count].item = old2;
            s = old2->type->slot;
            if (s == 4 || s == 9)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 5;
            else if (s == 5)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 6;
            cutscene_saved_equipment_count++;
        } else {
            item_free(old2);
        }
    }

    if (a->arg0c < 0 && c->equipment[9] != NULL) {
        item_info *off = character_unequip_slot(c, 9, 1);
        cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
        cutscene_saved_equipment[cutscene_saved_equipment_count].item = off;
        cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 9;
        cutscene_saved_equipment_count++;
    }
    return true;
}

// 0x4dce60  Confidence: likely (the early-out tests; D22704: the records indexed with the count as in
// replace_weapon, closer; left: Sum.exe keeps a zero register in EBX and `save` in the frame). Action 31: the
// same for the armour (slot 7; ItemType
// slots 6 / 8 save as 7, 7 as 8). Nothing happens when the new type is a slot-6 item (or "(none)")
// and the worn armour is not a slot-8 item, or when both are slot-8 items. When the slots 7 / 8 were
// empty, records with a NULL item note that for the rewind.
bool script_action_replace_armor(script_action *a, Character *c)
{
    item_info *old1 = NULL;
    item_info *old2 = NULL;
    item_info *it;
    bool had7 = c->equipment[7] != NULL;
    bool save = true;
    bool had8 = c->equipment[8] != NULL;
    int i;
    int s;

    if (c->equipment[7] != NULL) {
        if (c->equipment[7]->type->slot == 8) {
            if (item_types[a->arg0c].slot == 8)
                return true;
        } else {
            if (item_types[a->arg0c].slot == 6)
                return true;
        }
    } else {
        if (a->arg0c < 0)
            return true;
        if (item_types[a->arg0c].slot == 6)
            return true;
    }

    for (i = 0; i < cutscene_saved_equipment_count; i++) {
        if (cutscene_saved_equipment[i].c == c) {
            s = cutscene_saved_equipment[i].slot;
            if (s == 7 || s == 8) {
                save = false;
                break;
            }
        }
    }

    if (a->arg0c >= 0) {
        it = item_create(a->arg0c);
        character_equip_item(&old1, &old2, c, 7, it);
    } else {
        it = NULL;
        old1 = character_unequip_slot(c, 7, 1);
    }

    if (old1 != NULL) {
        if (save) {
            cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
            cutscene_saved_equipment[cutscene_saved_equipment_count].item = old1;
            s = old1->type->slot;
            if (s == 6 || s == 8)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 7;
            else if (s == 7)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 8;
            cutscene_saved_equipment_count++;
        } else {
            item_free(old1);
        }
    }
    if (old2 != NULL) {
        if (save) {
            cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
            cutscene_saved_equipment[cutscene_saved_equipment_count].item = old2;
            s = old2->type->slot;
            if (s == 6 || s == 8)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 7;
            else if (s == 7)
                cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 8;
            cutscene_saved_equipment_count++;
        } else {
            item_free(old2);
        }
    }

    if (it != NULL) {
        if (!had7) {
            cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
            cutscene_saved_equipment[cutscene_saved_equipment_count].item = NULL;
            cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 7;
            cutscene_saved_equipment_count++;
        }
        if (!had8) {
            cutscene_saved_equipment[cutscene_saved_equipment_count].c = c;
            cutscene_saved_equipment[cutscene_saved_equipment_count].item = NULL;
            cutscene_saved_equipment[cutscene_saved_equipment_count].slot = 8;
            cutscene_saved_equipment_count++;
        }
    }
    return true;
}

// 0x4dd0b0  Confidence: exact (VC6 byte match, D22705: `ok = true` with an explicit `case -1: return;` and no
// default (the table starts at -1, other types fall out with ok set), the cases in the binary's code order, the
// wander flag passed as the bool it is, the next action through a pointer, the move found before the second
// _stricmp, the name copied by an if / else of two strcpy; was likely: jump table 0x4dd698 over type + 1; the
// cases that finish at once return without the retry message). Starts the current action; an action that cannot start formats "Unable
// to start script entry ..." (into a local buffer that is never shown) and retries in 5 s.
void script_entry_start(script_object *so, Character *c)
{
    char name[64];
    char msg[256];
    script_action *a;
    bool ok = true;   // D22705
    bool hold;
    move_info *m;

    if (so->end_mode == -1 || so->cur_action < 0)
        return;

    so->timer.invalidate();
    a = &so->actions[so->cur_action];
    switch (a->type) {
    case -1:
        return;

    case SCRIPT_ACTION_WALK_TO:
    case SCRIPT_ACTION_RUN_TO:
    case SCRIPT_ACTION_MOVE_THROUGH:
        ok = script_action_walk_to(c, so);
        break;

    case SCRIPT_ACTION_WAIT_AT:
        ok = script_action_wait_at(c, so);
        break;

    case SCRIPT_ACTION_WANDER:
    case SCRIPT_ACTION_WANDER_FOR:
        ok = script_action_wander(c, a, *(bool *)&a->flag14);
        if (a->type == SCRIPT_ACTION_WANDER_FOR)
            so->timer.SetTimeout(a->arg10 * 1000);
        break;

    case SCRIPT_ACTION_DO_STATE:
        ok = script_action_do_state(c, a);
        break;

    case SCRIPT_ACTION_PLAY_ACTION:
    case SCRIPT_ACTION_PLAY_MVF:
        if (c->type == 9 && so->cur_action > 0) {
            int prev = so->actions[so->cur_action - 1].type;
            if (prev == SCRIPT_ACTION_MOUNT_ANIM)
                boss_set_ride_anims(c);
            else if (prev == SCRIPT_ACTION_DISMOUNT_ANIM)
                boss_set_foot_anims(c);
        }
        if (so->cur_action + 1 < so->num_actions) {
            script_action *na = &so->actions[so->cur_action + 1];
            if (na->type == SCRIPT_ACTION_SYNCH || na->type == SCRIPT_ACTION_SYNCH_TELEPORT) {
                ok = script_action_play_action(c, a, true);
            } else {
                hold = false;
                if ((game_flags & 0x20) && c->info->species == 0
                    && _stricmp(cutscene_data->name, "game-intro") == 0
                    && (m = charinfo_find_move_any(c->info, a->arg0c, true),
                        _stricmp(m->action, "gi-Josp-Sneak-Ready.mvf") == 0))
                    hold = true;
                ok = script_action_play_action(c, a, hold);
            }
        } else {
            ok = script_action_play_action(c, a, false);
        }
        break;

    case SCRIPT_ACTION_PLAY_EFFECT:
        fx_attach_to_character(a->text, c->handle, -1, -1, 0, 0);
        script_entry_advance(so, c);
        return;

    case SCRIPT_ACTION_TURN_HOSTILE:
        if (a->arg0c) {
            if (species_is_boss(c->info->species))
                c->type = 9;
            else
                c->type = 5;
            return;
        }
        c->type = 4;
        return;

    case SCRIPT_ACTION_NO_BLOCK:
        if (a->arg0c)
            c->flags |= 0x20;
        else
            c->flags &= ~0x20;
        return;

    case SCRIPT_ACTION_SAY:
        c->say(a->text, (float)a->arg0c);
        return;

    case SCRIPT_ACTION_DIE:
        ok = script_action_die(c, a);
        so->end_mode = -1;
        break;

    case SCRIPT_ACTION_GOTO:
        ok = script_action_goto(c, a, so);
        break;

    case SCRIPT_ACTION_TELEPORT_TO:
        c->flags &= ~0x1000;
        c->pos = a->nav->pos;
        c->orient.set_from_angles(0.0f, 0.0f, a->nav->heading);
        c->layer = a->nav->layer;
        c->find_ground();
        script_entry_advance(so, c);
        return;

    case SCRIPT_ACTION_PASS_GO:
        script_npc_send_go(a->text);
        script_entry_advance(so, c);
        return;

    case SCRIPT_ACTION_FOLLOW:
        ok = script_action_follow(so, c, a->text, a->arg0c);
        break;

    case SCRIPT_ACTION_SHOW_HIDE:
        c->set_ai_active(a->arg0c);
        if (a->arg0c) {
            c->clear_flag_2_4();
            script_entry_advance(so, c);
            return;
        }
        c->set_flag_2_4(-1);
        script_entry_advance(so, c);
        return;

    case SCRIPT_ACTION_SYNCH_TELEPORT:
        c->flags &= ~0x1000;
        c->pos = a->nav->pos;
        c->orient.set_from_angles(0.0f, 0.0f, a->nav->heading);
        c->layer = a->nav->layer;
        c->find_ground();
        // fall through
    case SCRIPT_ACTION_SYNCH:
        c->instance->clear_action_anims();
        c->instance->clear_state_anims();
        c->update_locomotion_anim();
        c->instance->process(0.0f, false, 0);
        if (script_entry_check_done(so, c))
            script_entry_advance(so, c);
        return;

    case SCRIPT_ACTION_COMBAT_MODE:
        ok = script_action_combat_mode(a, so, c);
        break;

    case SCRIPT_ACTION_MOUNT_ANIM:
    case SCRIPT_ACTION_DISMOUNT_ANIM:
        script_entry_advance(so, c);
        return;

    case SCRIPT_ACTION_REPLACE_WEAPON:
        ok = script_action_replace_weapon(a, c);
        break;

    case SCRIPT_ACTION_REPLACE_ARMOR:
        ok = script_action_replace_armor(a, c);
        break;

    case SCRIPT_ACTION_GUARD_SPLINE:
        if (a->arg0c) {
            script_alloc_avoid_spline(c->handle, cur_character->handle);
            return;
        }
        script_free_avoid_spline(c->handle);
        return;

    case SCRIPT_ACTION_ONE_HAND:
        c->set_armour_alt_texture(a->arg0c != 0, 0);
        return;

    }

    if (!ok) {
        Character *o = living_from_handle(so->handle);
        if (o != NULL)
            strcpy(name, o->info->name);
        else
            strcpy(name, XSTR("Unknown", 352));
        sprintf(msg, "Unable to start script entry %d for %s.  Try again in 5 seconds\n", so->cur_action, name);
        so->timer.SetTimeout(5000);
    }
}

// 0x4dd720  Confidence: exact. Past the last action: end mode 0 starts over (cur_action -1, the next
// advance runs action 0), 1 frees the object (-1), 2 (cutscene) stays at the end.
void script_entry_advance(script_object *so, Character *c)
{
    if (so->cur_action < -1)
        so->cur_action = -1;
    so->cur_action++;
    if (so->cur_action >= so->num_actions) {
        switch (so->end_mode) {
        case 0:
            so->cur_action = -1;
            return;
        case 1:
            so->end_mode = -1;
            return;
        }
        return;
    }
    script_entry_start(so, c);
}

// 0x4dd780  Confidence: exact (VC6 byte match, D22702: one exit for the switch (the EDI push is shrink-wrapped
// into it), the cases in the binary's code order, the "not done" cases as `done = false; break;` with play
// action jumping into follow's copy, the next action through a pointer, `done = true` before the flag store;
// was likely: switch through the byte table 0x4dda08 / jump table 0x4dd9d4: the
// actions that finish when started share the "true" case; "wander", "die" return false, "play effect"
// and unknown types fall to the end with false). An action followed by a synch waits while a cutscene
// is loaded.
bool script_entry_check_done(script_object *so, Character *c)
{
    bool done = false;
    script_action *a;
    int prev;

    if (so->cur_action < 0)
        return true;
    if (so->end_mode == 2 && so->cur_action >= so->num_actions)
        return false;
    if (cutscene_data != NULL && so->cur_action < so->num_actions - 1) {
        script_action *na = &so->actions[so->cur_action + 1];
        if (na->type == SCRIPT_ACTION_SYNCH || na->type == SCRIPT_ACTION_SYNCH_TELEPORT)
            return false;
    }

    a = &so->actions[so->cur_action];
    switch (a->type) {
    case SCRIPT_ACTION_WALK_TO:
    case SCRIPT_ACTION_RUN_TO:
        done = c->ai.goal_find(9, AI_GOAL_GOTO_NAVPOINT, 0, 0, -1, &a->nav->pos, a->nav->layer, AI_NO_SPELL) == -1;
        break;

    case SCRIPT_ACTION_PLAY_ACTION:
    case SCRIPT_ACTION_PLAY_MVF:
        if (a->arg0c >= 0 && c->instance->get_anim_time_left(a->arg0c) > reported_frametime)
            goto not_done;
        done = true;
        c->flags &= ~0x1000;
        if (c->type == 9 && so->cur_action > 0) {
            prev = so->actions[so->cur_action - 1].type;
            if (prev == SCRIPT_ACTION_MOUNT_ANIM)
                boss_script_hook_stub(c);
            else if (prev == SCRIPT_ACTION_DISMOUNT_ANIM)
                boss_dismount_dispatch(c);
        }
        break;

    case SCRIPT_ACTION_WAIT_GO:
        done = a->arg0c != 0;
        if (done == true)
            ai_group_remove_member(c);
        break;

    case SCRIPT_ACTION_TRIGGER:
        done = script_object_near_player_dist(c, (float)a->arg0c);
        break;

    case SCRIPT_ACTION_TRIGGER_NAV:
        done = script_navpoint_near_player_dist(a->nav, (float)a->arg0c);
        break;

    case SCRIPT_ACTION_TRIGGER_VISIT:
        done = script_check_visit_count(a->arg0c);
        break;

    case -1:
    case SCRIPT_ACTION_MOVE_THROUGH:
    case SCRIPT_ACTION_WAIT_AT:
    case SCRIPT_ACTION_DO_STATE:
    case SCRIPT_ACTION_TURN_HOSTILE:
    case SCRIPT_ACTION_SAY:
    case SCRIPT_ACTION_LABEL:
    case SCRIPT_ACTION_GOTO:
    case SCRIPT_ACTION_NO_BLOCK:
    case SCRIPT_ACTION_TELEPORT_TO:
    case SCRIPT_ACTION_PASS_GO:
    case SCRIPT_ACTION_SHOW_HIDE:
    case SCRIPT_ACTION_MOUNT_ANIM:
    case SCRIPT_ACTION_DISMOUNT_ANIM:
    case SCRIPT_ACTION_REPLACE_WEAPON:
    case SCRIPT_ACTION_GUARD_SPLINE:
    case SCRIPT_ACTION_REPLACE_ARMOR:
    case SCRIPT_ACTION_ONE_HAND:
        done = true;
        break;

    case SCRIPT_ACTION_WANDER:
    case SCRIPT_ACTION_DIE:
        done = false;
        break;

    case SCRIPT_ACTION_WANDER_FOR:
        done = true;
        c->ai.goal_remove(c->ai.goal_find(1, AI_GOAL_WANDER, 0, 0, -1, NULL, 0, AI_NO_SPELL));
        break;

    case SCRIPT_ACTION_FOLLOW:
        if (!so->timer.valid()) {
        not_done:
            done = false;
            break;
        }
        done = true;
        c->ai.goal_remove(c->ai.goal_find(1, AI_GOAL_FOLLOW, 0, 0, -1, NULL, 0, AI_NO_SPELL));
        break;

    case SCRIPT_ACTION_SYNCH:
    case SCRIPT_ACTION_SYNCH_TELEPORT:
        done = cutscene_data == NULL;
        break;

    case SCRIPT_ACTION_COMBAT_MODE:
        done = c->ai.goal_find(1, AI_GOAL_SWITCH_COMBAT, 0, 0, -1, NULL, 0, AI_NO_SPELL) == -1;
        break;
    }
    return done;
}

// 0x4dda30  Confidence: exact (VC6 byte match) (the order of the squares in the sum). cur_character within dist of c.
bool script_object_near_player_dist(Character *c, float dist)
{
    if (c == NULL)
        return false;
    if (cur_character == NULL)
        return false;
    return (c->pos - cur_character->pos).len_sq() <= dist * dist;
}

// 0x4dda90  Confidence: exact (VC6 byte match) (as above).
bool script_navpoint_near_player_dist(navpoint *np, float dist)
{
    if (cur_character == NULL)
        return false;
    return (cur_character->pos - np->pos).len_sq() <= dist * dist;
}

// 0x4ddaf0  Confidence: exact.
bool script_check_visit_count(int count)
{
    return savegame_state.get_visit_count(cur_level_index) >= count;
}

// 0x4ddb10  Confidence: exact (VC6 byte match) (the search keeps the action pointer of the last iteration). A cutscene
// jumped to another sequence: the script continues after its synch action for that sequence (a synch
// teleport also places the character), with the AI goals, the animations and a move in progress reset.
void script_entry_jump_to_sequence(script_object *so, Character *c)
{
    script_action *a = NULL;
    script_action *cur;
    character_instance *inst;
    int i;
    int prev;

    for (i = 0; i < so->num_actions; i++) {
        a = &so->actions[i];
        if ((a->type == SCRIPT_ACTION_SYNCH || a->type == SCRIPT_ACTION_SYNCH_TELEPORT)
            && cutscene_cur_sequence == a->arg0c)
            break;
    }
    if (i >= so->num_actions)
        return;

    cur = &so->actions[so->cur_action];
    if (cur->type == SCRIPT_ACTION_PLAY_ACTION || cur->type == SCRIPT_ACTION_PLAY_MVF) {
        c->flags &= ~0x1000;
        if (c->type == 9 && so->cur_action > 0) {
            prev = so->actions[so->cur_action - 1].type;
            if (prev == SCRIPT_ACTION_MOUNT_ANIM)
                boss_script_hook_stub(c);
            else if (prev == SCRIPT_ACTION_DISMOUNT_ANIM)
                boss_dismount_dispatch(c);
        }
    }

    c->ai.goals_remove_unmarked();
    c->ai.go_idle();
    inst = c->instance;
    inst->clear_action_anims();
    inst->clear_state_anims();
    c->update_locomotion_anim();
    inst->process(0.0f, false, 0);

    if (a->type == SCRIPT_ACTION_SYNCH_TELEPORT) {
        c->pos = a->nav->pos;
        c->orient.set_from_angles(0.0f, 0.0f, a->nav->heading);
        c->layer = a->nav->layer;
    }
    so->cur_action = i;
    script_entry_advance(so, c);
}

// 0x4ddc60  Confidence: unsure (the two equal start points: the code builds the point half a unit
// towards the target twice and traces the second leg only when they differ, which never happens; the
// spline record's fields after the layer search). A guard spline (action 30) across the way from c to
// target: from the point 0.5 ahead of c, rays 20 units to the right and to the left along the
// orientation built from that direction; where a ray crosses a layer edge the spline is split 0.1 units
// on either side of the crossing. The point list is then repeated with side 2 (cell_x) for the other
// face; cell_y holds the layer of each piece.
int script_build_avoid_spline(Character *c, Character *target)
{
    vector3 dir;
    vector3 start;
    vector3 check;
    vector3 up;
    vector3 right_end, left_end;
    vector3 right_hit, left_hit;
    vector2 seg_a, seg_b, hit;
    seg2d seg;
    matrix m;
    avoid_spline_def def;
    float rx, rz, ox, oz;
    int layer, right_layer, left_layer;
    bool right_found, left_found;
    int n, i;

    dir.x = target->pos.x - c->pos.x;
    dir.y = 0.0f;
    dir.z = target->pos.z - c->pos.z;
    dir.normalize_safe();
    start = c->pos + dir * 0.5f;
    check = start;
    layer = pf_trace_edge_crossings(&c->pos, c->layer, check);
    if (!(check == start))
        layer = pf_trace_edge_crossings(&check, layer, start);

    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    m.set_from_forward_up(dir, up);

    rx = m.rvec.x * 20.0f;
    rz = m.rvec.z * 20.0f;
    seg.a = &seg_a;
    seg.b = &seg_b;
    right_found = false;
    right_hit.x = 0.0f;
    right_hit.z = 0.0f;
    right_layer = layer;
    seg_a.x = start.x;
    seg_a.y = start.z;
    right_end.x = start.x + rx;
    right_end.z = start.z + rz;
    seg_b.x = right_end.x;
    seg_b.y = right_end.z;
    left_end.x = start.x - rx;
    left_end.z = start.z - rz;
    if (pf_first_edge_crossing(&seg, layer, &hit, &right_layer)) {
        right_hit.x = hit.x;
        right_hit.z = hit.y;
        right_found = true;
    }

    seg_a.x = start.x;
    seg_a.y = start.z;
    seg_b.x = left_end.x;
    seg_b.y = left_end.z;
    left_found = false;
    left_layer = layer;
    left_hit.x = 0.0f;
    left_hit.z = 0.0f;
    if (pf_first_edge_crossing(&seg, layer, &hit, &left_layer)) {
        left_hit.x = hit.x;
        left_hit.z = hit.y;
        left_found = true;
    }

    n = 0;
    def.p0[0].x = right_end.x;
    def.p0[0].y = right_end.z;
    def.cell_y[0] = right_layer;
    def.cell_x[0] = 1;
    if (right_found == true) {
        ox = m.rvec.x * 0.1f;
        n = 1;
        def.cell_y[1] = layer;
        def.cell_x[1] = 1;
        def.p1[0].x = right_hit.x - ox;
        oz = m.rvec.z * 0.1f;
        def.p1[0].y = right_hit.z - oz;
        def.p0[1].x = ox + right_hit.x;
        def.p0[1].y = oz + right_hit.z;
    }
    if (left_found == true) {
        ox = m.rvec.x * 0.1f;
        n++;
        def.p1[n - 1].x = left_hit.x - ox;
        oz = m.rvec.z * 0.1f;
        def.p1[n - 1].y = left_hit.z - oz;
        def.p0[n].x = ox + left_hit.x;
        def.p0[n].y = oz + left_hit.z;
        def.cell_y[n] = layer;
        def.cell_x[n] = 1;
    }
    def.p1[n].x = left_end.x;
    def.p1[n].y = left_end.z;
    def.cell_y[n] = left_layer;
    def.cell_x[n] = 1;
    n++;

    for (i = 0; i < n; i++) {
        def.p0[n + i].x = def.p0[i].x;
        def.p0[n + i].y = def.p0[i].y;
        def.p1[n + i].x = def.p1[i].x;
        def.p1[n + i].y = def.p1[i].y;
        def.cell_y[n + i] = def.cell_y[i];
        def.cell_x[n + i] = 2;
    }
    def.count = n * 2;
    return avoid_spline_create(&def);
}
