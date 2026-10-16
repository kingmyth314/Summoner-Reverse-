// unnamed_dialogue_table_0x4feb50.cpp - Sum.exe v1.40, code 0x4feb50-0x500a22. Written from the Sum.exe
// disassembly (tools/asm.mjs; the three jump tables 0x500924 / 0x500938 / 0x500964 read from the image).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls,
// constants and globals match), likely (one detail of statement order or expression form is a guess), unsure
// (the shape itself is a guess).
//
// The conversation records' small members (speaker, topic, line, action) and the dialogue table parser,
// <script>_dlg.tbl into conversation_db (conversation::load calls conversation::parse_table).
// FILE NAME NOT KNOWN (D5310): no path string, no strings.tbl block, no reference name; a placeholder. Its
// .data 0x5a5484-0x5a5728 follows level_script_wolong.cpp's with no shared literal, and the vsdk\os mouse
// code starts at 0x500a30 (source-map.md). The members are the record types' methods (declared with the
// records in levelscripts/conversation.h); conversation.cpp (0x4d7b30-) is a different unit.
//
// Header: levelscripts/conversation.h.

#include <string.h>
#include <stdio.h>
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/parse/parse.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// Topic references whose target is not known yet while the table is read (names ours). The topic of the
// current speaker named before its +Topic: (resolved after the speaker), a topic of another speaker (and
// a speaker) resolved at the end, and a speaker named before its $Character: (only checked at the end).
struct dialogue_pending_topic {          // 0xc bytes, pool at esp+0xa4
    conversation_topic_ref *ref;         // +0x0
    char *name;                          // +0x4
    dialogue_pending_topic *next;        // +0x8
};
struct dialogue_pending_ext_topic {      // 0x10 bytes, pool at esp+0x88
    conversation_topic_ref *ref;         // +0x0
    char *topic;                         // +0x4
    char *speaker;                       // +0x8
    dialogue_pending_ext_topic *next;    // +0xc
};
struct dialogue_pending_speaker {        // 0xc bytes, pool at esp+0xc0
    conversation_speaker_ref *ref;       // +0x0
    char *name;                          // +0x4
    dialogue_pending_speaker *next;      // +0x8
};

// ---- conversation_speaker / conversation_topic ----------------------------------------------------------

// 0x4feb50  Confidence: exact. Move-in 3 s, move-out 1.5 s, no flags, no topics.
void conversation_speaker::init(const char *name)
{
    next = NULL;
    flags = 0;
    move_in_time = 3000;
    move_out_time = 1500;
    this->name = name;
    topics = NULL;
    topics_tail = NULL;
    greeting = NULL;
}

// 0x4feb80  Confidence: exact (the inline strcmp). The topic of that label, NULL.
conversation_topic *conversation_speaker::find_topic(const char *name)
{
    for (conversation_topic *t = topics; t; t = t->next)
        if (!strcmp(t->name, name))
            return t;
    return NULL;
}

// 0x4febe0  Confidence: exact. Every topic is offered again (listed and fresh) when it is the greeting or
// an initial topic, and withdrawn otherwise.
void conversation_speaker::refresh_topic_flags()
{
    for (conversation_topic *t = topics; t; t = t->next) {
        if (t == greeting || t->initial)
            t->flags |= CTOPIC_LISTED | CTOPIC_FRESH;
        else
            t->flags &= ~(CTOPIC_LISTED | CTOPIC_FRESH);
    }
}

// 0x4fec10  Confidence: exact.
void conversation_topic::init(const char *name, const char *text)
{
    this->name = name;
    this->text = text;
    lines = NULL;
    lines_tail = NULL;
    next = NULL;
    flags = 0;
    add_topics = NULL;
    remove_topics = NULL;
    add_initial_topics = NULL;
    remove_initial_topics = NULL;
    hide_topics = NULL;
    unhide_topics = NULL;
    hide_quests = NULL;
    unhide_quests = NULL;
    set_important = NULL;
    clear_important = NULL;
    set_hail_topics = NULL;
}

// 0x4fec50  Confidence: exact. Appends to the topic list.
void conversation_speaker::add_topic(conversation_topic *topic)
{
    if (topics) {
        topics_tail->next = topic;
        topics_tail = topic;
        topic->next = NULL;
    } else {
        topics = topic;
        topics_tail = topic;
        topic->next = NULL;
    }
}

// (no address of its own) Confidence: likely. The same body for the topic's lines (+0x10 / +0x14, next +0);
// the linker kept one copy, 0x4fec50, which parse_table calls with the topic in ECX (D5309).
void conversation_topic::add_line(conversation_line *line)
{
    if (lines) {
        lines_tail->next = line;
        lines_tail = line;
        line->next = NULL;
    } else {
        lines = line;
        lines_tail = line;
        line->next = NULL;
    }
}

// 0x4fec80  Confidence: exact. The eleven reference lists push on the front.
void conversation_topic::push_add_topic(conversation_topic_ref *r)
{
    r->next = add_topics;
    add_topics = r;
}

// 0x4fec90  Confidence: exact.
void conversation_topic::push_remove_topic(conversation_topic_ref *r)
{
    r->next = remove_topics;
    remove_topics = r;
}

// 0x4feca0  Confidence: exact.
void conversation_topic::push_add_initial_topic(conversation_topic_ref *r)
{
    r->next = add_initial_topics;
    add_initial_topics = r;
}

// 0x4fecb0  Confidence: exact.
void conversation_topic::push_remove_initial_topic(conversation_topic_ref *r)
{
    r->next = remove_initial_topics;
    remove_initial_topics = r;
}

// 0x4fecc0  Confidence: exact.
void conversation_topic::push_hide_topic(conversation_topic_ref *r)
{
    r->next = hide_topics;
    hide_topics = r;
}

// 0x4fecd0  Confidence: exact.
void conversation_topic::push_unhide_topic(conversation_topic_ref *r)
{
    r->next = unhide_topics;
    unhide_topics = r;
}

// 0x4fece0  Confidence: exact.
void conversation_topic::push_hide_quest(conversation_quest_ref *r)
{
    r->next = hide_quests;
    hide_quests = r;
}

// 0x4fecf0  Confidence: exact.
void conversation_topic::push_unhide_quest(conversation_quest_ref *r)
{
    r->next = unhide_quests;
    unhide_quests = r;
}

// 0x4fed00  Confidence: exact.
void conversation_topic::push_set_important(conversation_speaker_ref *r)
{
    r->next = set_important;
    set_important = r;
}

// 0x4fed10  Confidence: exact.
void conversation_topic::push_clear_important(conversation_speaker_ref *r)
{
    r->next = clear_important;
    clear_important = r;
}

// 0x4fed20  Confidence: exact.
void conversation_topic::push_set_hail_topic(conversation_topic_ref *r)
{
    r->next = set_hail_topics;
    set_hail_topics = r;
}

// ---- the parser ------------------------------------------------------------------------------------------

// 0x4fed30  Confidence: exact (Cflag_None ors the strcmp result 0). The quoted flag names after +Flags:.
int conversation::parse_cflags(TextFile *tf)
{
    char name[0x100];
    int result = 0;
    int flag;

    while (tf->Peek("\"")) {
        tf->ParseString(name, 0x100, '"', '"');
        if (!_stricmp(name, "Cflag_None"))
            flag = 0;
        else if (!_stricmp(name, "Cflag_DontFacePlayer"))
            flag = CFLAG_DONT_FACE_PLAYER;
        else if (!_stricmp(name, "Cflag_NoCameraMovement"))
            flag = CFLAG_NO_CAMERA_MOVEMENT;
        else if (!_stricmp(name, "Cflag_NoAbort"))
            flag = CFLAG_NO_ABORT;
        else if (!_stricmp(name, "Cflag_Important"))
            flag = CFLAG_IMPORTANT;
        else if (!_stricmp(name, "Cflag_Persistent"))
            flag = CFLAG_PERSISTENT;
        else {
            tf->Close();
            return -1;
        }
        result |= flag;
    }
    return result;
}

// The line kinds of a topic (+PCText: .. +JekharText:), CLINE_*; false when none follows. The parser's two
// copies of this test (before the first line and after each) are a rotated while loop over an inline
// function (D5311; name ours).
static inline bool dialogue_accept_line_kind(TextFile *tf, int *kind)
{
    if (tf->Accept("+PCText:"))
        *kind = CLINE_PC;
    else if (tf->Accept("+NPCText:"))
        *kind = CLINE_NPC;
    else if (tf->Accept("+JosephText:"))
        *kind = CLINE_JOSEPH;
    else if (tf->Accept("+RosalindText:"))
        *kind = CLINE_ROSALIND;
    else if (tf->Accept("+FleceText:"))
        *kind = CLINE_FLECE;
    else if (tf->Accept("+JekharText:"))
        *kind = CLINE_JEKHAR;
    else
        return false;
    return true;
}

// The topic reference kinds (+Add topic: .. +Set hail topic:, 0..10); the two important kinds take no {text}
// argument (no_arg). As above, inline and expanded twice (D5311; name ours).
static inline bool dialogue_accept_ref_kind(TextFile *tf, int *kind, bool *no_arg)
{
    if (tf->Accept("+Add topic:"))
        *kind = 0;
    else if (tf->Accept("+Remove topic:"))
        *kind = 1;
    else if (tf->Accept("+Add initial topic:"))
        *kind = 2;
    else if (tf->Accept("+Remove initial topic:"))
        *kind = 3;
    else if (tf->Accept("+Hide topic:"))
        *kind = 4;
    else if (tf->Accept("+Unhide topic:"))
        *kind = 5;
    else if (tf->Accept("+Hide quest:"))
        *kind = 6;
    else if (tf->Accept("+Unhide quest:"))
        *kind = 7;
    else if (tf->Accept("+Set important")) {
        *kind = 8;
        *no_arg = true;
    } else if (tf->Accept("+Clear important")) {
        *kind = 9;
        *no_arg = true;
    } else if (tf->Accept("+Set hail topic:"))
        *kind = 10;
    else
        return false;
    return true;
}

// 0x4fee50  Confidence: unsure (the shape of the whole: which tests are inline helpers, loops or if
// chains; the calls, constants, record layouts and the order of every check are from the code, D5311).
// Reads <script_name>_dlg.tbl into this conversation: $Character: blocks (speaker name, move times, +Flags:,
// +Topic: blocks with +Shop / +Hidden, the lines with their actions, the topic references, +Farewell),
// a "Farewell." topic when no topic has bit 0, $Initial topics: (the first is the greeting), $End. Returns
// TextFile::Open's error, -1 for a bad table and 0 for a good one (xor eax, eax at 0x5008a6, after the
// destructors; conversation::load treats 0 as success).
int conversation::parse_table(const char *script_name)
{
    char filename[0x100];
    char name[0x100];
    char text[0x400];
    char speaker_name[0x100];
    char ref_speaker[0x100];
    char quest_name[0x100];
    char pending_buffer[0x2000];
    char ext_buffer[0x2000];
    char speaker_buffer[0x2000];
    NoRlsAllocPool pending_pool;
    NoRlsAllocPool ext_pool;
    NoRlsAllocPool speaker_pool;
    dialogue_pending_topic *pending = NULL;
    dialogue_pending_ext_topic *pending_ext = NULL;
    dialogue_pending_speaker *pending_speakers = NULL;
    int num_topics = 0;
    TextFile tf;
    conversation_speaker *speaker;
    conversation_topic *topic;
    conversation_line *line;
    dialogue_action *a;
    const char *who;
    char *s;
    char *t;
    int kind;
    int value;
    int species;
    int i;
    int err;

    strcpy(filename, script_name);
    strcat(filename, "_dlg.tbl");
    pending_pool.Init(pending_buffer, 0x2000);
    ext_pool.Init(ext_buffer, 0x2000);
    speaker_pool.Init(speaker_buffer, 0x2000);
    err = tf.Open(filename, 9999999);
    if (err != 0)
        return err;

    while (tf.Accept("$Character:")) {
        if (!tf.ParseString(name, 0x100, '"', '"'))
            { tf.Close(); return -1; }
        if (has_speaker(name) || has_speaker_hash(name_hash_nocase(name)))
            { tf.Close(); return -1; }
        i = find_name(name);
        if (i < 0) {
            s = (char *)pool.Alloc(strlen(name) + 1);
            strcpy(s, name);
            add_name(s);
            speaker = alloc_speaker(s);
        } else {
            speaker = alloc_speaker(names[i]);
        }
        if (tf.Accept("+Move in time:"))
            speaker->move_in_time = (ushort)tf.ParseInt();
        if (tf.Accept("+Move out time:"))
            speaker->move_out_time = (ushort)tf.ParseInt();
        if (tf.Accept("+Flags:"))
            speaker->flags = (ushort)parse_cflags(&tf);

        while (tf.Accept("+Topic:")) {
            unsigned int hash;
            bool no_arg;

            if (!tf.ParseString(name, 0x100, '{', '}') || speaker->find_topic(name))
                { tf.Close(); return -1; }
            hash = name_hash_nocase(name);
            for (topic = speaker->topics; topic; topic = topic->next)
                if (hash == (unsigned int)name_hash_nocase(topic->name))
                    { tf.Close(); return -1; }
            // The label; the text the player says is the label up to a '#'.
            s = (char *)pool.Alloc(strlen(name) + 1);
            strcpy(s, name);
            t = strchr(name, '#');
            if (t) {
                int len = t - name;
                char *label = (char *)pool.Alloc(len + 1);
                strncpy(label, name, len);
                label[t - name] = '\0';
                topic = alloc_topic(s, label, false);
            } else {
                topic = alloc_topic(s, s, false);
            }
            for (;;) {
                if (tf.Accept("+Shop"))
                    topic->flags |= CTOPIC_SHOP;
                else if (tf.Accept("+Hidden"))
                    topic->flags |= CTOPIC_HIDDEN;
                else
                    break;
            }

            // The lines and their actions.
            while (dialogue_accept_line_kind(&tf, &kind)) {
                line = (conversation_line *)pool.Alloc(sizeof(conversation_line));
                who = NULL;
                switch (kind) {
                case CLINE_NPC:
                    if (tf.Peek("\"")) {
                        if (!tf.ParseString(name, 0x100, '"', '"'))
                            { tf.Close(); return -1; }
                        i = find_name(name);
                        if (i < 0) {
                            // A name not seen before is pooled but not used: the line keeps who = NULL.
                            s = (char *)pool.Alloc(strlen(name) + 1);
                            strcpy(s, name);
                            add_name(s);
                        } else {
                            who = names[i];
                        }
                    } else {
                        who = speaker->name;
                    }
                    break;
                case CLINE_JOSEPH:
                    who = species_table[SPECIES_JOSEPH].name;
                    break;
                case CLINE_ROSALIND:
                    who = species_table[SPECIES_ROSALIND].name;
                    break;
                case CLINE_FLECE:
                    who = species_table[SPECIES_FLECE].name;
                    break;
                case CLINE_JEKHAR:
                    who = species_table[SPECIES_JEKHAR].name;
                    break;
                }
                if (!tf.ParseString(text, 0x400, '{', '}'))
                    { tf.Close(); return -1; }
                localize_remap_charset(text, 1);
                s = (char *)pool.Alloc(strlen(text) + 1);
                strcpy(s, text);
                line->init(s, kind, who);

                for (;;) {
                    if (tf.Accept("+Trigger:")) {
                        tf.ParseString(name, 0x100, '"', '"');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 0, 0, -1);
                        line->append_action(a);
                    } else if (tf.Accept("+Event:")) {
                        tf.ParseString(name, 0x100, '"', '"');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 6, 0, -1);
                        line->append_action(a);
                    } else if (tf.Accept("+Unhide by item:")) {
                        tf.ParseString(name, 0x100, '{', '}');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 7, 0, -1);
                        tf.ParseString(name, 0x100, '"', '"');
                        a->value = item_type_find(name);
                        line->append_action(a);
                    } else if (tf.Accept("+Unhide by gold:")) {
                        tf.ParseString(name, 0x100, '{', '}');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 11, 0, -1);
                        a->value = tf.ParseInt();
                        line->append_action(a);
                    } else if (tf.Accept("+Unhide by event:")) {
                        tf.ParseString(name, 0x100, '{', '}');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 8, 0, -1);
                        tf.ParseString(name, 0x100, '"', '"');
                        a->value = level_script_find(name);
                        tf.ParseString(name, 0x100, '"', '"');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a->flag = s;
                        line->append_action(a);
                    } else if (tf.Accept("+Hide by item:")) {
                        tf.ParseString(name, 0x100, '{', '}');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 9, 0, -1);
                        tf.ParseString(name, 0x100, '"', '"');
                        a->value = item_type_find(name);
                        line->append_action(a);
                    } else if (tf.Accept("+Hide by gold:")) {
                        tf.ParseString(name, 0x100, '{', '}');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 12, 0, -1);
                        a->value = tf.ParseInt();
                        line->append_action(a);
                    } else if (tf.Accept("+Hide by event:")) {
                        tf.ParseString(name, 0x100, '{', '}');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init(s, 10, 0, -1);
                        tf.ParseString(name, 0x100, '"', '"');
                        a->value = level_script_find(name);
                        tf.ParseString(name, 0x100, '"', '"');
                        s = (char *)pool.Alloc(strlen(name) + 1);
                        strcpy(s, name);
                        a->flag = s;
                        line->append_action(a);
                    } else if (tf.Accept("+AddXP:")) {
                        value = tf.ParseInt();
                        species = -1;
                        if (tf.Peek("\"")) {
                            tf.ParseString(name, 0x100, '"', '"');
                            for (i = 0; i < 4; i++)
                                if (!_stricmp(name, species_table[i].name)) {
                                    species = species_table[i].species;
                                    break;
                                }
                        }
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init("AdjustXP", 1, value, species);
                        line->append_action(a);
                    } else if (tf.Accept("+AddSP:")) {
                        // The member's name is required here (no Peek).
                        value = tf.ParseInt();
                        species = -1;
                        tf.ParseString(name, 0x100, '"', '"');
                        for (i = 0; i < 4; i++)
                            if (!_stricmp(name, species_table[i].name)) {
                                species = species_table[i].species;
                                break;
                            }
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init("AdjustSP", 2, value, species);
                        line->append_action(a);
                    } else if (tf.Accept("+AdjustGP:")) {
                        value = tf.ParseInt();
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init("AdjustGP", 3, value, -1);
                        line->append_action(a);
                    } else if (tf.Accept("+Gain item:")) {
                        tf.ParseString(name, 0x100, '"', '"');
                        value = item_type_find(name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init("Gain Item", 4, value, -1);
                        line->append_action(a);
                    } else if (tf.Accept("+Lose item:")) {
                        tf.ParseString(name, 0x100, '"', '"');
                        value = item_type_find(name);
                        a = (dialogue_action *)pool.Alloc(sizeof(dialogue_action));
                        a->init("Lose Item", 5, value, -1);
                        line->append_action(a);
                    } else
                        break;
                }
                topic->add_line(line);
            }

            // The topic references: each kind takes any number of {topic} / {quest} arguments, the two
            // important kinds none (they run once).
            no_arg = false;
            while (dialogue_accept_ref_kind(&tf, &kind, &no_arg)) {
                while (tf.Peek("{") || no_arg) {
                    int target = -1;
                    bool qualified = false;

                    no_arg = false;
                    switch (kind) {
                    case 0:
                    case 2:
                        tf.ParseString(text, 0x400, '{', '}');
                        target = 0;
                        qualified = tf.Accept("To:");
                        break;
                    case 1:
                    case 3:
                    case 4:
                    case 5:
                        tf.ParseString(text, 0x400, '{', '}');
                        target = 0;
                        qualified = tf.Accept("From:");
                        break;
                    case 6:
                    case 7:
                        tf.ParseString(text, 0x400, '{', '}');
                        target = 1;
                        break;
                    case 8:
                    case 9:
                        target = 2;
                        qualified = tf.Accept(":");
                        break;
                    case 10:
                        tf.ParseString(text, 0x400, '{', '}');
                        target = 0;
                        qualified = tf.Accept("For:");
                        break;
                    }
                    switch (target) {
                    case 0: {
                        conversation_topic_ref *r = (conversation_topic_ref *)pool.Alloc(sizeof(conversation_topic_ref));
                        r->next = NULL;
                        if (qualified) {
                            conversation_speaker *other;
                            tf.ParseString(ref_speaker, 0x100, '"', '"');
                            other = find_speaker(ref_speaker);
                            if (!other) {
                                dialogue_pending_ext_topic *d;
                                r->topic = NULL;
                                r->speaker = NULL;
                                d = (dialogue_pending_ext_topic *)ext_pool.Alloc(sizeof(dialogue_pending_ext_topic));
                                d->topic = (char *)ext_pool.Alloc(strlen(text) + 1);
                                d->speaker = (char *)ext_pool.Alloc(strlen(ref_speaker) + 1);
                                strcpy(d->topic, text);
                                strcpy(d->speaker, ref_speaker);
                                d->ref = r;
                                d->next = pending_ext;
                                pending_ext = d;
                            } else {
                                r->speaker = other;
                                r->topic = other->find_topic(text);
                                if (!r->topic)
                                    { tf.Close(); return -1; }
                            }
                        } else {
                            r->speaker = speaker;
                            r->topic = speaker->find_topic(text);
                            if (!r->topic) {
                                dialogue_pending_topic *d;
                                d = (dialogue_pending_topic *)pending_pool.Alloc(sizeof(dialogue_pending_topic));
                                d->name = (char *)pending_pool.Alloc(strlen(text) + 1);
                                strcpy(d->name, text);
                                d->ref = r;
                                d->next = pending;
                                pending = d;
                            }
                        }
                        switch (kind) {
                        case 0:
                            topic->push_add_topic(r);
                            break;
                        case 1:
                            topic->push_remove_topic(r);
                            break;
                        case 2:
                            topic->push_add_initial_topic(r);
                            break;
                        case 3:
                            topic->push_remove_initial_topic(r);
                            break;
                        case 4:
                            topic->push_hide_topic(r);
                            break;
                        case 5:
                            topic->push_unhide_topic(r);
                            break;
                        case 10:
                            topic->push_set_hail_topic(r);
                            break;
                        }
                        break;
                    }
                    case 1: {
                        int quest;
                        trim_whitespace(quest_name, text, 0xff);
                        memmove(quest_name, quest_name + 1, strlen(quest_name) - 1);
                        quest_name[strlen(quest_name) - 2] = '\0';
                        quest = quest_find_by_name(quest_name);
                        if (quest >= 0) {
                            conversation_quest_ref *r = (conversation_quest_ref *)pool.Alloc(sizeof(conversation_quest_ref));
                            r->next = NULL;
                            r->quest = quest;
                            switch (kind) {
                            case 6:
                                topic->push_hide_quest(r);
                                break;
                            case 7:
                                topic->push_unhide_quest(r);
                                break;
                            }
                        } else if (savegame_state.used > 0) {
                            printf("Illegal quest name : %s\n", quest_name);
                        }
                        break;
                    }
                    case 2: {
                        conversation_speaker_ref *r = (conversation_speaker_ref *)pool.Alloc(sizeof(conversation_speaker_ref));
                        r->next = NULL;
                        if (qualified) {
                            tf.ParseString(speaker_name, 0x100, '"', '"');
                            r->speaker = find_speaker(speaker_name);
                            if (!r->speaker) {
                                dialogue_pending_speaker *d;
                                d = (dialogue_pending_speaker *)speaker_pool.Alloc(sizeof(dialogue_pending_speaker));
                                d->name = (char *)speaker_pool.Alloc(strlen(speaker_name) + 1);
                                strcpy(d->name, speaker_name);
                                d->next = pending_speakers;
                                pending_speakers = d;
                                d->ref = r;
                            }
                        } else {
                            r->speaker = speaker;
                        }
                        switch (kind) {
                        case 8:
                            topic->push_set_important(r);
                            break;
                        case 9:
                            topic->push_clear_important(r);
                            break;
                        }
                        break;
                    }
                    }
                }
            }

            if (tf.Accept("+Farewell"))
                topic->farewell = 1;
            if (!topic->lines && !topic->farewell && !topic->shop)
                { tf.Close(); return -1; }
            speaker->add_topic(topic);
            num_topics++;
        }

        // Without a topic of bit 0 the speaker gets (or turns into initial farewells) "Farewell.".
        for (topic = speaker->topics; topic; topic = topic->next)
            if (topic->flag0)
                break;
        if (!topic) {
            topic = speaker->find_topic("\"Farewell.\"");
            if (topic) {
                topic->flags |= CTOPIC_FAREWELL | CTOPIC_INITIAL;
            } else {
                s = (char *)pool.Alloc(strlen("\"Farewell.\"") + 1);
                strcpy(s, "\"Farewell.\"");
                topic = alloc_topic(s, s, false);
                topic->flags |= CTOPIC_FAREWELL | CTOPIC_INITIAL;
                speaker->add_topic(topic);
                num_topics++;
            }
        }
        // The references to this speaker's later topics.
        for (dialogue_pending_topic *p = pending; p; p = p->next) {
            topic = speaker->find_topic(p->name);
            if (!topic)
                { tf.Close(); return -1; }
            p->ref->topic = topic;
            p->ref->speaker = speaker;
        }
        pending = NULL;
        pending_pool.Reset();

        tf.Expect("$Initial topics:");
        while (tf.Peek("{")) {
            tf.ParseString(name, 0x100, '{', '}');
            topic = speaker->find_topic(name);
            if (!topic)
                { tf.Close(); return -1; }
            topic->flags |= CTOPIC_INITIAL;
            if (!speaker->greeting)
                speaker->greeting = topic;
        }
        add_speaker(speaker);
    }
    tf.Expect("$End");
    tf.Close();

    // The references to other speakers' topics; the speaker references are only checked (their speaker
    // stays NULL, see NOTES).
    for (dialogue_pending_ext_topic *e = pending_ext; e; e = e->next) {
        conversation_speaker *other = find_speaker(e->speaker);
        if (!other)
            return -1;
        topic = other->find_topic(e->topic);
        if (!topic)
            return -1;
        e->ref->topic = topic;
        e->ref->speaker = other;
    }
    for (dialogue_pending_speaker *p = pending_speakers; p; p = p->next)
        if (!find_speaker(p->name))
            return -1;
    return 0;
}

// 0x500990  Confidence: exact. +4 (the flag name) is left as it is.
void dialogue_action::init(const char *name, int kind, int value, int species)
{
    this->name = name;
    this->kind = kind;
    this->value = value;
    this->species = species;
    next = NULL;
}

// 0x5009c0  Confidence: exact.
void conversation_line::init(const char *text, int kind, const char *who)
{
    this->text = text;
    this->kind = kind;
    this->who = who;
    next = NULL;
    actions = NULL;
}

// 0x5009f0  Confidence: exact. Appends to the action list.
void conversation_line::append_action(dialogue_action *a)
{
    if (actions) {
        dialogue_action *p = actions;
        while (p->next)
            p = p->next;
        p->next = a;
        a->next = NULL;
    } else {
        actions = a;
        a->next = NULL;
    }
}
