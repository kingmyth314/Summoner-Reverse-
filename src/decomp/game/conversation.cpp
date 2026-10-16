// conversation.cpp - Sum.exe v1.40, code 0x4d7b30-0x4d87ce (levelscripts\conversation.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The conversation object conversation_db (0x25461a0, class conversation): the level's conversation file
// (speakers, topics, lines, parsed by dialogue_parse_table 0x4fee50 into the 128 KB pool +0x210), the
// speaker and topic of the running conversation, the offered-topic list (16 entries) and the topic state
// in the savegame. Unit range: static_init_conversation_25461a0 (0x4d7b30, initialiser table entry 369)
// to conversation_farewell's RET (0x4d87ce); conversationctl.cpp starts at 0x4d87d0 (entry 370).
// Data: .data 0x5a0250-0x5a02cb (the path, the error text, "\"Farewell.\"" - the last also read by
// dialogue_parse_table, pooled), .bss conversation_db 0x25461a0-0x25464af. strings.tbl id 467 "farewell".

#include <string.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// ---- .bss ----------------------------------------------------------------------------------------
// 0x4d7b30 static_init_conversation_25461a0 (call 0x4d7b40; jmp 0x4d7b50) /
// 0x4d7b40 init_conversation_25461a0 (mov ecx, 0x25461a0; jmp conversation__ctor) /
// 0x4d7b50 register_atexit_conversation_25461a0 (atexit(0x4d7b60)) /
// 0x4d7b60 atexit_dtor_conversation_25461a0
// (mov ecx, 0x25461a0; jmp 0x4d7b70) are the compiler's dynamic initialiser and atexit helpers of this
// definition;
// 0x4d7b70 conversation__dtor is the implicit destructor (add ecx, 0x210; jmp
// NoRlsAllocPool__dtor: the pool is the only member with a destructor). Confidence: exact.
conversation conversation_db;                        // 0x25461a0 (tagged new_saveload.cpp in sum.symbols)

// 0x4d7b80  Confidence: likely (EH frame for the pool member; the two list heads' prev stores come first in
// the binary, list_init writes next first - scheduling).
conversation::conversation()
{
    int i;

    level = -1;
    clear();
    list_init(&topic_list);
    list_init(&free_entries);
    for (i = 0; i < 16; i++)
        list_append(&free_entries, &entries[i]);
}

// 0x4d7c20  Confidence: exact. Called once from GameInit; InitBlocks returns nonzero when the block could
// not be allocated. Line 0x131 = 305.
int conversation::alloc_storage()
{
    if (pool.InitBlocks(0x20000, false))
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\levelscripts\\conversation.cpp", 305,
                   "Unable to allocate space for conversation storage");
    empty_function();
    level = -1;
    return 0;
}

// 0x4d7c70  Confidence: exact (tail jump to NoRlsAllocPool::Reset).
void conversation::clear()
{
    level = -1;
    name_count = 0;
    speaker = NULL;
    topic = NULL;
    speakers = NULL;
    pool.Reset();
}

// 0x4d7ca0  Confidence: exact. From game_cleanup.
void conversation::free()
{
    clear();
    pool.Release();
}

// 0x4d7cc0  Confidence: exact.
void conversation::add_speaker(conversation_speaker *s)
{
    s->next = speakers;
    speakers = s;
}

// 0x4d7ce0  Confidence: exact.
conversation_speaker *conversation::alloc_speaker(const char *name)
{
    conversation_speaker *s = (conversation_speaker *)pool.Alloc(sizeof(conversation_speaker));
    s->init(name);
    return s;
}

// 0x4d7d10  Confidence: likely (`cmp byte ptr [esp+0x10], 1`: the bool is compared with true).
conversation_topic *conversation::alloc_topic(const char *name, const char *text, bool listed)
{
    conversation_topic *t = (conversation_topic *)pool.Alloc(sizeof(conversation_topic));
    t->init(name, text);
    if (listed == true)
        t->flags |= CTOPIC_LISTED | CTOPIC_FRESH;
    return t;
}

// 0x4d7d50  Confidence: likely (the two stores between the list test are scheduled; the loop moving the
// offered entries back to the free list is written with the list macros).
int conversation::set_speaker(const char *name)
{
    conversation_speaker *s = find_speaker(name);
    conversation_topic_entry *e;

    if (s == NULL)
        return -1;
    speaker = s;
    if (s->greeting == NULL) {
        speaker = NULL;
        return -1;
    }
    if (s->greeting->hidden) {
        speaker = NULL;
        return -1;
    }
    s->refresh_topic_flags();
    topic = NULL;
    journal_updated = 0;
    for (e = topic_list.next; e != &topic_list; e = topic_list.next) {
        list_remove(e);
        list_append(&free_entries, e);
    }
    build_topic_list();
    return 0;
}

// 0x4d7e10  Confidence: exact.
void conversation::clear_speaker()
{
    if (speaker) {
        empty_function();
        speaker = NULL;
        topic = NULL;
    }
}

// 0x4d7e40  Confidence: exact.
void conversation::set_topic(conversation_topic *t)
{
    if (t && t->listed && !t->hidden) {
        t->listed = 0;
        topic = t;
        apply_topic_effects();
        remove_topic_entry(t);
    }
}

// 0x4d7e90  Confidence: exact.
conversation_topic *conversation::select_greeting()
{
    conversation_topic *t = speaker->greeting;
    if (t)
        set_topic(t);
    return t;
}

// 0x4d7eb0  Confidence: likely (the loop order and bit operations match; the constants held in
// registers are the compiler's). Game bug kept: the inner loop of +Remove initial topic: steps with the
// OUTER entry's next pointer (`mov ecx, [eax + 8]` with eax = the outer entry), so when the removed topic
// is its speaker's hail topic and the topic also has +Add initial topic: entries, the loop never ends unless
// the outer entry is the last one; the hail topic becomes the outer next entry's topic each time.
void conversation::apply_topic_effects()
{
    conversation_topic *t;
    conversation_topic_ref *r, *m;
    conversation_quest_ref *q;
    conversation_speaker_ref *s;

    for (t = speaker->topics; t; t = t->next)
        t->fresh = 0;

    for (r = topic->set_hail_topics; r; r = r->next) {
        r->speaker->greeting = r->topic;
        r->topic->initial = 1;
    }
    for (r = topic->add_topics; r; r = r->next) {
        r->topic->fresh = 1;
        r->topic->listed = 1;
    }
    for (r = topic->add_initial_topics; r; r = r->next) {
        r->topic->fresh = 1;
        r->topic->initial = 1;
    }
    for (r = topic->remove_topics; r; r = r->next) {
        r->topic->listed = 0;
        remove_topic_entry(r->topic);
    }
    for (r = topic->remove_initial_topics; r; r = r->next) {
        r->topic->initial = 0;
        r->topic->listed = 0;
        if (r->topic == r->speaker->greeting) {
            for (m = topic->add_initial_topics; m; m = r->next)
                r->speaker->greeting = m->topic;
        }
    }
    for (r = topic->hide_topics; r; r = r->next)
        r->topic->hidden = 1;
    for (r = topic->unhide_topics; r; r = r->next)
        r->topic->hidden = 0;

    for (q = topic->hide_quests; q; q = q->next) {
        journal_updated = 1;
        quest_journal_set_entry(q->quest, 0);
    }
    for (q = topic->unhide_quests; q; q = q->next) {
        journal_updated = 1;
        quest_journal_set_entry(q->quest, 1);
    }

    for (s = topic->set_important; s; s = s->next) {
        if (s->speaker)
            s->speaker->flags |= CFLAG_IMPORTANT;
    }
    for (s = topic->clear_important; s; s = s->next) {
        if (s->speaker)
            s->speaker->flags &= ~CFLAG_IMPORTANT;
    }
}

// 0x4d80c0  Confidence: exact.
bool conversation::has_speaker(const char *name)
{
    conversation_speaker *s;

    for (s = speakers; s; s = s->next) {
        if (!_stricmp(name, s->name))
            return true;
    }
    return false;
}

// 0x4d8100  Confidence: exact.
bool conversation::has_speaker_hash(unsigned int hash)
{
    conversation_speaker *s;

    for (s = speakers; s; s = s->next) {
        if (hash == name_hash_nocase(s->name))
            return true;
    }
    return false;
}

// 0x4d8140  Confidence: exact.
conversation_topic *conversation::first_topic()
{
    conversation_topic *t;

    for (t = speaker->topics; t; t = t->next) {
        if (t->listed && !t->hidden)
            return t;
    }
    return NULL;
}

// 0x4d8170  Confidence: exact (thiscall, `this` not used).
conversation_topic *conversation::next_topic(conversation_topic *t)
{
    for (t = t->next; t; t = t->next) {
        if (t->listed && !t->hidden)
            return t;
    }
    return NULL;
}

// 0x4d81a0  Confidence: likely (the count is reloaded after the store; `name_count++` then `- 1`).
int conversation::add_name(char *name)
{
    names[name_count] = name;
    name_count++;
    return name_count - 1;
}

// 0x4d81d0  Confidence: exact (unsigned compare: name_count is unsigned).
int conversation::find_name(const char *name)
{
    unsigned int i;

    for (i = 0; i < name_count; i++) {
        if (!_stricmp(names[i], name))
            return i;
    }
    return -1;
}

// 0x4d8220  Confidence: exact. The console commands dialogue_make_topic_(un)available and
// conversation_execute_action call it. Making the hail topic unavailable moves the hail to the first
// visible initial topic.
int conversation::set_topic_available(const char *speaker_name, const char *topic_name, bool available)
{
    conversation_speaker *s = find_speaker(speaker_name);
    conversation_topic *t, *g;

    if (s == NULL)
        return -1;
    t = s->find_topic(topic_name);
    if (t == NULL)
        return -1;
    if (available) {
        t->hidden = 0;
        return 0;
    }
    t->hidden = 1;
    if (t == s->greeting) {
        for (g = s->topics; g; g = g->next) {
            if (!g->hidden && g->initial) {
                s->greeting = g;
                break;
            }
        }
    }
    return 0;
}

// 0x4d82b0  Confidence: exact.
conversation_speaker *conversation::find_speaker(const char *name)
{
    conversation_speaker *s;

    for (s = speakers; s; s = s->next) {
        if (!_stricmp(s->name, name))
            return s;
    }
    return NULL;
}

// 0x4d82f0  Confidence: likely (the three returns are separate epilogues in the binary).
int conversation::load(const char *level_name, const char *script_name)
{
    int result = parse_table(script_name);

    if (result == 0) {
        level = level_script_find(level_name);
        slot = level_script_find_index(level, script_name);
        if (savegame_state.has_conversation_data(level, slot))
            restore_state();
        else
            savegame_state.init_conversation_slot(level, slot, speakers);
    } else {
        level = -1;
    }
    return result;
}

// 0x4d8390  Confidence: exact (tail jump into conversation_save_topics).
void conversation::save_state()
{
    if (level != -1)
        save_topics();
}

// 0x4d83a0  Confidence: likely (the speaker bits are read from the 16-bit flag word as `>> n & 1`; the
// topic bits as bitfields). One speaker bit (important) and four topic bits: 0 flag0, 1 initial,
// 2 hidden, 3 the hail topic. Persistent speakers write to the level's other slots too.
void conversation::save_topics()
{
    conversation_speaker *s;
    conversation_topic *t;

    for (s = speakers; s; s = s->next) {
        bool persistent = (s->flags >> 4) & 1;
        savegame_state.set_speaker_bit(level, slot, s->name, 0, (s->flags >> 3) & 1, persistent);
        for (t = s->topics; t; t = t->next) {
            savegame_state.set_topic_bit(level, slot, s->name, t->name, 0, t->flag0, persistent);
            savegame_state.set_topic_bit(level, slot, s->name, t->name, 1, t->initial, persistent);
            savegame_state.set_topic_bit(level, slot, s->name, t->name, 2, t->hidden, persistent);
            savegame_state.set_topic_bit(level, slot, s->name, t->name, 3, s->greeting == t, persistent);
        }
    }
}

// 0x4d84d0  Confidence: likely (if / else per bit: the binary branches to an `or` or an `and` of the flag
// word; the important bit is or byte / and word on the 16-bit flags).
void conversation::restore_state()
{
    conversation_speaker *s;
    conversation_topic *t;

    for (s = speakers; s; s = s->next) {
        if (savegame_state.get_speaker_bit(level, slot, s->name, 0))
            s->flags |= CFLAG_IMPORTANT;
        else
            s->flags &= ~CFLAG_IMPORTANT;
        for (t = s->topics; t; t = t->next) {
            int bit0 = savegame_state.get_topic_bit(level, slot, s->name, t->name, 0);
            int bit1 = savegame_state.get_topic_bit(level, slot, s->name, t->name, 1);
            int bit2 = savegame_state.get_topic_bit(level, slot, s->name, t->name, 2);
            int bit3 = savegame_state.get_topic_bit(level, slot, s->name, t->name, 3);
            if (bit0)
                t->flag0 = 1;
            else
                t->flag0 = 0;
            if (bit1)
                t->initial = 1;
            else
                t->initial = 0;
            if (bit2)
                t->hidden = 1;
            else
                t->hidden = 0;
            if (bit3)
                s->greeting = t;
        }
    }
}

// 0x4d8620  Confidence: likely (inline strcmp; list_insert's last two stores are swapped in the binary).
// Every fresh topic takes a free entry; the "Farewell." topic goes to the end, the others to the front.
void conversation::build_topic_list()
{
    conversation_topic *t;
    conversation_topic_entry *e;

    for (t = first_topic(); t; t = next_topic(t)) {
        if (t->fresh) {
            e = free_entries.next;
            list_remove(e);
            e->topic = t;
            if (strcmp(t->name, "\"Farewell.\"") != 0)
                list_insert(&topic_list, e);
            else
                list_append(&topic_list, e);
        }
    }
}

// 0x4d86f0  Confidence: exact.
void conversation::remove_topic_entry(conversation_topic *t)
{
    conversation_topic_entry *e;

    for (e = topic_list.next; e != &topic_list; e = e->next) {
        if (e->topic == t) {
            list_remove(e);
            list_append(&free_entries, e);
            return;
        }
    }
}

// 0x4d8760  Confidence: likely (the localized word is a local: it is loaded once before both loops).
// Fills the topic box: the topics whose text does not contain "farewell" first, then those that do.
void conversation::farewell()
{
    const char *word = XSTR("farewell", 467);
    conversation_topic_entry *e;

    for (e = topic_list.next; e != &topic_list; e = e->next) {
        if (!stristr(e->topic->text, word))
            dialogue_topic_box_add_kind3(e->topic);
    }
    for (e = topic_list.next; e != &topic_list; e = e->next) {
        if (stristr(e->topic->text, word))
            dialogue_topic_box_add_kind3(e->topic);
    }
}
