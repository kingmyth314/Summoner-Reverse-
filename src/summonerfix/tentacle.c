// Experimental: the Tentacle Beast's grab and throw (docs/future/tentacle-grab.md, game-bugs G250 and G251).
// Off by default: [Experimental] TentacleGrab=1 turns it on, TentacleGrabChance (percent, default 10) sets how
// often a tentacle that may grab tries to. [Debug] TraceTentacle=1 logs grab picks, grabs, hold ticks, releases,
// throws, landings and drops with the record index (0..3), the victim's handle and HP.
//
// The four tentacles of the sewer boss (records of 0x110 bytes at 0x248c328) carry a complete grab / hold / throw
// mechanic, with its moves in Tentacle Beast.tbl ("Luminar grab" tent_pickup.mvf, "Luminar throw"
// tent_pickup_throw.mvf, "pick up ready") and the heroes' "flail" and "throwndown" moves, but it never runs:
// tentacle_beast_ai_select_action picks the grab only when frand() < 0.0 (0x4c666d..0x4c667f, 0x57eb84 is the
// program's shared 0.0) and then clears the pick (0x4c66a5) and lets the swipe search overwrite it (0x4c67de). The
// gate tentacle_beast_grab_ready (0x248c770; 1 at load, 0 after a grab, 1 again 10 s after a landing, 0 while only
// one party member lives, kept by tentacle_beast_battle_update 0x4c7640) is maintained but read by nothing. The
// dead code has slips of its own, fixed here (numbers are the slip list of the plan, section 5):
//
// 1. Select (0x4c6520, thiscall (ai) -> 3 attack / 4 nothing in reach / 5 busy; entry jmp, 83 EC 34 53 55).
//    TentacleSelectAction is the original, step for step (busy tests, the 2.5..5 s think timer, up to five
//    candidates: a live slot handle, then living type-7 characters without 0x88; frand ladder: < 0.0 grab (dead,
//    kept), < 0.3 squeeze (nearest candidate within 3.0 of the squeeze point on X/Z), else swipe (first candidate
//    whose edge distance is below the reach +0xe4)). Before that ladder, while grab_ready is 1, not in multiplayer
//    (game_flags 0x5fbfd8 bit 2) and frand() < chance, it looks for a candidate that has "flail" (14, 0x8c) and
//    "throwndown" (8, 1) and no 0x88: the nearest within 3.0 of the pickup point on X/Z ("$tag-26-pickup", read
//    live, else +0xac from load), else the first within reach. A pick stores move 0x86 (0x248c7a4) in +0x7c, the
//    target in +0x8c, sets grab_ready 0 (one grab at a time) and returns 3, so tentacle_beast_ai_frame calls
//    tentacle_beast_start_grab (0x4c72b6..0x4c72cf). Slips 1, 2, 3. Also a watchdog for slip 4 (below).
// 2. Grab (0x4c6850, thiscall (ai, victim), ret 4; entry jmp, 53 56 8B 35 CC). TentacleAiGrab is the original
//    plus: the trigger counts only when character_move_trigger_fired returns 1 (it returns -1 for a move without
//    the trigger, which the original took as fired; slip 8); the victim must be alive and not held (no 0x88) and
//    within the reach + 1.0 at the impact frame (slip 5); the release HP mark +0xe0 is the tentacle's HP x 0.75,
//    not the victim's (0x4c6912 read [victim+0x5f8], compared with the tentacle's HP at 0x4c74b1; slip 7), so the
//    victim is let go when the tentacle has lost a quarter of its HP, or after the 30 s hold; the "pick up ready"
//    lookup is NULL-tested (slip 10); the leadership moves off the victim only when it leads (party_leader
//    0x5fcc80 or cur_character 0x5fcc88), to the first other member that is alive and not held (slip 9). When the
//    grab move ends with nothing held, the grab timer (0x248c768) is armed for 10 s so the gate opens again: the
//    original left grab_ready 0 and the timer invalid until a landing (slip 4).
// 3. Hold tick (0x4c714b in tentacle_beast_ai_frame): push 0x40400000 (+3.0, a heal: 68 00 00 40 40) becomes
//    push 0xC0400000 (-3.0, 3 damage every 6 s; slip 6). With TraceTentacle the call after it (0x4c7150,
//    E8 -> character_add_hp) goes through TentacleTickAddHp, which logs the tick and calls character_add_hp.
// 4. Release (0x4c6a80, thiscall (ai); entry jmp, 83 EC 48 56 57). TentacleAiRelease is the original (at trigger 8
//    the held victim becomes the thrown one, velocity (swipe point - victim) x 2/3, gravity 0, the tentacle's ready
//    anims back, the camera follows the victim if it is cur_character) with the trigger tested for 1 only and a
//    NULL / dead test on the victim (the original read [NULL+0x10] at 0x4c6b53; slip 8): no victim -> nothing
//    thrown, the tentacle's anims back, the grab timer armed.
// 5. Battle end (0x4c6c50 tentacle_begin_sink, cdecl (record, ms); entry jmp, 53 57 8B 7C 24). The original only
//    called tentacle_beast_start_release for a held victim and returned without sinking; ls_sewerboss_frame then
//    clears tentacle_beast_battle_started (0x4ee217), after which tentacle_beast_ai_frame never reaches the
//    release, so the victim stayed on the tentacle with flags 0x1028 (AI off) for good, and a tentacle with a
//    flying victim never sank (slip 12). TentacleBeginSink puts a held or flying victim down at once
//    (TentacleDrop) and then runs the original sink: sink timer, flag 4, the party's attack goals on this
//    tentacle removed. A record without a character is left alone (the original crashed on it).
// 6. Frame hook (0x4c7960, cdecl (tentacle); entry jmp + nop, 81 EC A8 00 00 00). TentacleFrameHook is the
//    original (the held victim 1.5 below the grab bone with its orientation; the thrown one flies by its velocity
//    with a ray against floor objects (lobj flag 0x40), stands 0.5 above a hit, horizontal speed x (1 - 0.75 dt),
//    vertical speed - gravity dt, gravity + 9.8 dt; on landing flags &= ~0x1028, ai_go_idle, face level,
//    character_find_ground, the grab timer 10 s, "throwndown") plus: a held or flying victim that is gone, dead
//    (0x80) or at 0 HP is put down at once (TentacleDrop; the original kept a corpse on the bone and woke it with
//    ai_go_idle on landing; slip 11). 0x8 cannot tell a dying victim from a held one (the grab sets it), hence HP; a landing spot off the navigation mesh is moved to the nearest navigable point
//    (pf_point_is_navigable 0x4a5dc0 / pf_find_nearest_navigable_point 0x49e8c0, as luminar_frame_hook does at
//    0x4c92a8); a body that falls 3.0 below both its start and its aim point (into the water) or flies for 3 s is
//    landed the same way (slip 13).
// 7. G251 (0x4c7870 in tentacle_collect_area_hits): hits[num_hits++] = handle had no bound; the record has four
//    slots (+0x94), so a fifth hit wrote over the death move +0xa4 and the tentacle could not die. The 29 bytes
//    become mov edx, [edi+0x90] ; cmp edx, 4 ; jge 0x4c788d ; mov eax, [ebp] ; mov [edi+4*edx+0x94], eax ; inc edx ;
//    mov [edi+0x90], edx ; nop. This one is patched on its own; the others are checked together first and patched
//    only if every site holds the expected bytes.
//
// TentacleDrop (battle end, gone or dead victims): clears 0x1028 (a body at 0 HP not yet dead keeps 0x8, dying),
// ai_go_idle unless down, faces level, moves to a navigable point, character_find_ground, "throwndown" if alive; the record's +0xbc / +0xcc = -1, the hold tick
// timer invalid, the tentacle's ready anims back (0x93 / 0x95), the grab timer 10 s.
//
// Character offsets used: pos +0x10, orient +0x38 (rows +0x38 / +0x44 / +0x50), type +0xc, handle +0x80, instance
// +0x314, next in the living list +0x318, flags +0x320 (0x80 dead, 0x8 dying or held, 0x20 / 0x1000 held), AI
// +0x324, combat +0x558, ready anims +0x5b8 / +0x5d0, HP +0x5f8, nav layer +0x68c, info +0x71c (nav size +0x90).
// AI (Character +0x324): character +0x4c, own move +0x194.
#include "summonerfix.h"
#include <string.h>

#define THISCALL __attribute__((thiscall))

// Patch sites.
#define VA_SELECT_ACTION 0x4c6520u    // tentacle_beast_ai_select_action
#define VA_AI_GRAB 0x4c6850u          // tentacle_beast_ai_grab
#define VA_HOLD_TICK_PUSH 0x4c714bu   // push 0x40400000 in tentacle_beast_ai_frame
#define VA_HOLD_TICK_CALL 0x4c7150u   // call character_add_hp right after it
#define VA_AI_RELEASE 0x4c6a80u       // tentacle_beast_ai_release
#define VA_BEGIN_SINK 0x4c6c50u       // tentacle_begin_sink
#define VA_FRAME_HOOK 0x4c7960u       // tentacle_beast_frame_hook
#define VA_HIT_STORE 0x4c7870u        // hits[num_hits++] in tentacle_collect_area_hits

// Game data.
#define VA_TENTACLES 0x248c328u       // 4 records of 0x110 bytes
#define VA_GRAB_TIMER 0x248c768u      // GameTimer
#define VA_GRAB_READY 0x248c770u      // int
#define VA_MOVE_1 0x248c794u          // swipe move
#define VA_MOVE_2 0x248c320u          // squeeze move
#define VA_MOVE_86 0x248c7a4u         // grab move ("Luminar grab")
#define VA_ACTIVE_TENTACLE 0x248c7a8u // record of the tentacle being run
#define VA_AI_MOVE_FLAGS 0x22e48ccu
#define VA_GAME_FLAGS 0x5fbfd8u       // bit 2: multiplayer
#define VA_PARTY_LIST 0x5fc350u       // list head; record +0 next, +0xc handle
#define VA_PARTY_LEADER 0x5fcc80u     // party record *
#define VA_CUR_CHARACTER 0x5fcc88u    // Character *
#define VA_LIVING_LIST 0x8eca60u      // list head (a dummy character); next at +0x318
#define VA_FRAMETIME 0x5a5740u        // float reported_frametime (s)
#define VA_GAME_CAMERA 0x60a018u

// Constants of the original code, read where it reads them.
#define VA_F_ZERO 0x57eb84u           // 0.0: the grab rung of the ladder
#define VA_F_SQUEEZE_CHANCE 0x581970u // 0.3
#define VA_F_NEAR 0x57ef1cu          // 3.0: squeeze (and grab) distance from the tag point
#define VA_F_RELEASE_PART 0x580e04u   // 0.75: release HP mark, and the throw's horizontal drag
#define VA_F_THROW_SCALE 0x580204u    // 2/3: throw velocity per unit of offset
#define VA_F_HOLD_BELOW 0x580210u     // 1.5: held victim below the grab bone
#define VA_F_HALF 0x57e6b0u           // 0.5: thrown victim above the floor hit
#define VA_F_ONE 0x57e990u            // 1.0
#define VA_F_GRAVITY 0x581c2cu        // 9.8

// Game functions.
#define VA_LIVING_FROM_HANDLE 0x46b1a0u
#define VA_FRAND 0x5426e0u
#define VA_COMBAT_IS_BUSY 0x468740u
#define VA_MOVE_PLAYING 0x461450u
#define VA_TIMER_VALID 0x501990u
#define VA_TIMER_ELAPSED 0x501910u
#define VA_TIMER_INVALIDATE 0x501900u
#define VA_TIMER_SET 0x501880u
#define VA_TIMER_SET_RANDOM 0x5018d0u
#define VA_DISTANCE 0x524810u
#define VA_EDGE_DISTANCE 0x454330u
#define VA_CHARINFO_FIND_MOVE 0x446cb0u
#define VA_FIND_MOVE 0x461370u
#define VA_PLAY_MOVE 0x460f20u
#define VA_MOVE_FINISHED 0x4614d0u
#define VA_TRIGGER_FIRED 0x461740u
#define VA_AI_GO_IDLE 0x4738c0u
#define VA_AI_IS_MODE 0x473200u
#define VA_CLEAR_STATE_ANIMS 0x51b100u
#define VA_SET_STATE_WEIGHT 0x51ae90u
#define VA_SET_FLAG_2_4 0x454670u
#define VA_CLEAR_FLAG_2_4 0x4546b0u
#define VA_UPDATE_LOCOMOTION 0x45a120u
#define VA_PARTY_SET_LEADER 0x433d60u
#define VA_CAMERA_SET_TARGET 0x478ff0u
#define VA_RAYCAST_ALL 0x4af2c0u
#define VA_BONE_WORLD 0x51b3f0u
#define VA_FIND_MESH_TAG 0x540f70u
#define VA_SET_FORWARD_UP 0x511200u
#define VA_FIND_GROUND 0x455fb0u
#define VA_NAV_TEST 0x4a5dc0u
#define VA_NAV_NEAREST 0x49e8c0u
#define VA_ADD_HP 0x450850u
#define VA_GOAL_FIND 0x475eb0u
#define VA_GOAL_REMOVE 0x476300u
#define VA_TENTACLE_FIND 0x4c7330u

// Tentacle record fields.
#define REC_SIZE 0x110
#define REC_MOVE 0x7c          // chosen move
#define REC_CHARACTER 0x80     // the tentacle
#define REC_FLAGS 0x84         // 1 down, 2 rising, 4 sinking
#define REC_TARGET 0x8c        // target handle
#define REC_THINK_TIMER 0xa8
#define REC_PICKUP 0xac        // "$tag-26-pickup" at load (world)
#define REC_GRAB_TAG 0xb8      // "$tag-grab-point"
#define REC_VICTIM 0xbc        // held handle
#define REC_HOLD_TIMER 0xc4    // 30 s
#define REC_TICK_TIMER 0xc8    // 6 s
#define REC_THROWN 0xcc        // thrown handle
#define REC_VELOCITY 0xd0
#define REC_GRAVITY 0xdc
#define REC_RELEASE_HP 0xe0
#define REC_REACH 0xe4
#define REC_SWIPE 0xe8
#define REC_SQUEEZE 0xf4
#define REC_SINK_TIMER 0x100

// Character fields.
#define CH_TYPE 0xc
#define CH_POS 0x10
#define CH_ORIENT 0x38
#define CH_UP 0x44
#define CH_FORWARD 0x50
#define CH_HANDLE 0x80
#define CH_INSTANCE 0x314
#define CH_NEXT 0x318
#define CH_FLAGS 0x320
#define CH_AI 0x324
#define CH_COMBAT 0x558
#define CH_READY_ANIM 0x5b8
#define CH_READY_HURT_ANIM 0x5d0
#define CH_HP 0x5f8
#define CH_NAV_LAYER 0x68c
#define CH_INFO 0x71c
#define INFO_NAV_SIZE 0x90
#define INST_CA_CHARACTER 0x3c
#define AI_CHARACTER 0x4c
#define AI_OWN_MOVE 0x194

#define FLAG_HELD 0x1028       // 0x8 dying / held, 0x20, 0x1000 (character_find_ground does nothing)
#define FLAG_DEAD 0x80
#define FLAG_DEAD_OR_DYING 0x88

#define NUM_TENTACLES 4
#define GRAB_COOLDOWN_MS 10000 // as the landing arms it (0x4c7c28)
#define FLIGHT_TIMEOUT 3.0f    // s
#define FALL_LIMIT 3.0f        // below both the throw's start and its aim point

#define I32(p, off) (*(int *)((BYTE *)(p) + (off)))
#define U32(p, off) (*(DWORD *)((BYTE *)(p) + (off)))
#define F32(p, off) (*(float *)((BYTE *)(p) + (off)))
#define PTR(p, off) (*(BYTE **)((BYTE *)(p) + (off)))
#define VEC(p, off) ((float *)((BYTE *)(p) + (off)))

static DWORD s_delta; // Sum.exe load address - 0x400000
#define VA(a) ((UINT_PTR)((a) + s_delta))
#define GAME(type, a) ((type)VA(a))
#define GLOBAL(type, a) (*(volatile type *)VA(a))
#define CONSTF(a) (*(const float *)VA(a))

static int s_trace;
static float s_chance; // 0..1
#define TRACE(...) do { if (s_trace) Log(__VA_ARGS__); } while (0)

// Per record: the throw's start height and flight time (for the fall limit and the timeout).
static float s_throwStartY[NUM_TENTACLES], s_flightTime[NUM_TENTACLES];

// The ray and hit records of lobj_raycast_all (levelrender.h lobj_ray, lobj_hit).
typedef struct {
    float start[3], dir[3];
    float radius;           // +0x18
    int flags;              // +0x1c
    DWORD requireFlags;     // +0x20 lobj flags that must be set (0x40: floor objects)
    DWORD excludeFlags;     // +0x24
    float stopT;            // +0x28
    int face;               // +0x2c
    float minRadius;        // +0x30
    float localStart[3], localDir[3];
} LobjRay;
typedef struct {
    int numHits;
    float t;                // hit distance as a fraction of dir
    float pos[3], normal[3];
    int unk20, face, unk28;
} LobjHit;

// ---- Game function types ----
typedef BYTE *(__cdecl *LivingFromHandleFn)(int);
typedef float(__cdecl *FrandFn)(void);
typedef char(THISCALL *CombatBusyFn)(void *);
typedef char(THISCALL *MovePlayingFn)(void *, int, int);
typedef char(THISCALL *TimerTestFn)(void *);
typedef void(THISCALL *TimerVoidFn)(void *);
typedef void(THISCALL *TimerSetFn)(void *, int);
typedef void(THISCALL *TimerRandomFn)(void *, int, int);
typedef float(__cdecl *DistanceFn)(const float *, const float *);
typedef float(THISCALL *EdgeDistanceFn)(void *, void *, int, int);
typedef BYTE *(__cdecl *CharinfoFindMoveFn)(void *, int, int, int);
typedef int(THISCALL *FindMoveFn)(void *, int, int);
typedef int(THISCALL *PlayMoveFn)(void *, int, float, int, int, int);
typedef char(THISCALL *MoveFinishedFn)(void *, int);
typedef int(THISCALL *TriggerFiredFn)(void *, int, int);
typedef void(THISCALL *AiVoidFn)(void *);
typedef char(THISCALL *AiIsModeFn)(void *, int, int);
typedef void(THISCALL *SetStateWeightFn)(void *, int, float);
typedef void(THISCALL *CharIntFn)(void *, int);
typedef void(THISCALL *CharVoidFn)(void *);
typedef void(__cdecl *PartySetLeaderFn)(void *);
typedef void(THISCALL *CameraSetTargetFn)(void *, void *);
typedef void(__cdecl *RaycastAllFn)(LobjRay *, LobjHit *);
typedef void(THISCALL *BoneWorldFn)(void *, int, const void *, const void *, void *, void *);
typedef int(THISCALL *FindMeshTagFn)(void *, const char *);
typedef void(THISCALL *SetForwardUpFn)(void *, const float *, const float *);
typedef char(__cdecl *NavFn)(float *, int, int);
typedef char(THISCALL *AddHpFn)(void *, float, int, int, int);
typedef int(THISCALL *GoalFindFn)(void *, int, int, int, int, int, int, int, int);
typedef void(THISCALL *GoalRemoveFn)(void *, int);
typedef BYTE *(__cdecl *TentacleFindFn)(void *);

// ---- Small helpers ----
static BYTE *FromHandle(int handle) { return GAME(LivingFromHandleFn, VA_LIVING_FROM_HANDLE)(handle); }
static BYTE *ActiveRecord(void) { return GLOBAL(LPBYTE, VA_ACTIVE_TENTACLE); }
static int RecordIndex(const BYTE *rec) { return (int)((UINT_PTR)rec - VA(VA_TENTACLES)) / REC_SIZE; }
static int IndexOk(int i) { return i >= 0 && i < NUM_TENTACLES; }
static void TimerSet(void *timer, int ms) { GAME(TimerSetFn, VA_TIMER_SET)(timer, ms); }
static void TimerInvalidate(void *timer) { GAME(TimerVoidFn, VA_TIMER_INVALIDATE)(timer); }
static int TimerElapsed(void *timer) { return GAME(TimerTestFn, VA_TIMER_ELAPSED)(timer) != 0; }
static void AiGoIdle(void *ai) { GAME(AiVoidFn, VA_AI_GO_IDLE)(ai); }
static void ArmGrabTimer(void) { TimerSet((void *)VA(VA_GRAB_TIMER), GRAB_COOLDOWN_MS); }
static float Hp(const BYTE *ch) { return F32(ch, CH_HP); }
static int HandleOf(const BYTE *ch) { return I32(ch, CH_HANDLE); }
static float Frametime(void) { return GLOBAL(float, VA_FRAMETIME); }
static void AddMoveFlag4(void) { GLOBAL(DWORD, VA_AI_MOVE_FLAGS) |= 4; }
// Dead (0x80) or at 0 HP. A held victim always has 0x8 (set by the grab), so 0x8 cannot tell dying from held.
static int IsDown(const BYTE *ch) { return (U32(ch, CH_FLAGS) & FLAG_DEAD) || Hp(ch) <= 0.0f; }

// charinfo_find_move(info, category, id, 1): the move record (anim index at +8), or NULL.
static BYTE *InfoMove(BYTE *ch, int category, int id)
{
    return GAME(CharinfoFindMoveFn, VA_CHARINFO_FIND_MOVE)(PTR(ch, CH_INFO), category, id, 1);
}

// Can this character be held and thrown: alive, not held, with the "flail" pose and the "throwndown" move.
static int Grabbable(BYTE *ch)
{
    if (!ch || (U32(ch, CH_FLAGS) & FLAG_DEAD_OR_DYING))
        return 0;
    BYTE *flail = InfoMove(ch, 14, 0x8c);
    if (!flail || I32(flail, 8) == -1)
        return 0;
    return GAME(FindMoveFn, VA_FIND_MOVE)(ch, 8, 1) != 0;
}

// The tentacle's ready / ready-hurt anims: "pick up ready" (0x8d) while holding, "ready" / "ready hurt" (0x93 /
// 0x95) otherwise. The original did not test the lookups for NULL.
static void SetTentacleAnims(BYTE *tentacle, int readyId, int hurtId)
{
    BYTE *ready = InfoMove(tentacle, 14, readyId), *hurt = InfoMove(tentacle, 14, hurtId);
    if (ready)
        I32(tentacle, CH_READY_ANIM) = I32(ready, 8);
    if (hurt)
        I32(tentacle, CH_READY_HURT_ANIM) = I32(hurt, 8);
    GAME(CharVoidFn, VA_UPDATE_LOCOMOTION)(tentacle);
}

// matrix3__set_from_forward_up(orient, forward with y = 0, (0, 1, 0)), as the landing does.
static void FaceLevel(BYTE *ch)
{
    float fwd[3] = {VEC(ch, CH_FORWARD)[0], 0.0f, VEC(ch, CH_FORWARD)[2]};
    float up[3] = {0.0f, 1.0f, 0.0f};
    GAME(SetForwardUpFn, VA_SET_FORWARD_UP)(ch + CH_ORIENT, fwd, up);
}

// Moves the character to the nearest navigable point when it is off the navigation mesh. A body that fell below
// liftY (into the water) is lifted back to it, since character_find_ground looks for a floor from 10 above.
static void SnapToNav(BYTE *ch, float liftY)
{
    float *pos = VEC(ch, CH_POS);
    int size = I32(PTR(ch, CH_INFO), INFO_NAV_SIZE), layer = I32(ch, CH_NAV_LAYER);
    if (GAME(NavFn, VA_NAV_TEST)(pos, size, layer))
        return;
    GAME(NavFn, VA_NAV_NEAREST)(pos, size, layer);
    if (pos[1] < liftY)
        pos[1] = liftY;
}

// The landing of tentacle_beast_frame_hook (0x4c7bac..0x4c7c77) with the fixes: no ai_go_idle and no
// "throwndown" for a dead or dying body, and the spot moved onto the navigation mesh. A body at 0 HP that is not
// dead yet keeps 0x8 (dying), so its death goes on; the original cleared all of 0x1028.
static void PutDown(BYTE *ch, float liftY)
{
    int alive = !IsDown(ch);
    int dying = !alive && !(U32(ch, CH_FLAGS) & FLAG_DEAD);
    U32(ch, CH_FLAGS) &= ~(DWORD)(dying ? FLAG_HELD & ~8 : FLAG_HELD);
    if (alive)
        AiGoIdle(ch + CH_AI);
    FaceLevel(ch);
    SnapToNav(ch, liftY);
    GAME(CharVoidFn, VA_FIND_GROUND)(ch);
    int move = alive ? GAME(FindMoveFn, VA_FIND_MOVE)(ch, 8, 1) : 0;
    if (move) {
        int played = GAME(PlayMoveFn, VA_PLAY_MOVE)(ch, move, 1.0f, 0, 0, 0);
        GAME(CharIntFn, VA_SET_FLAG_2_4)(ch, played);
    } else {
        GAME(CharVoidFn, VA_CLEAR_FLAG_2_4)(ch);
    }
}

// Puts a held or thrown victim down now and clears the record (battle end, victim gone or dead).
static void TentacleDrop(BYTE *rec, const char *why)
{
    int idx = RecordIndex(rec);
    int handles[2] = {I32(rec, REC_VICTIM), I32(rec, REC_THROWN)};
    I32(rec, REC_VICTIM) = -1;
    I32(rec, REC_THROWN) = -1;
    for (int i = 0; i < 2; i++) {
        if (handles[i] == -1)
            continue;
        BYTE *ch = FromHandle(handles[i]);
        TRACE("trace tentacle %d: drop %s victim 0x%x (%s), HP %.1f", idx, i ? "thrown" : "held", handles[i], why,
              ch ? Hp(ch) : 0.0f);
        if (!ch)
            continue;
        float liftY = VEC(ch, CH_POS)[1];
        if (i == 1 && IndexOk(idx) && s_throwStartY[idx] > liftY)
            liftY = s_throwStartY[idx];
        PutDown(ch, liftY);
    }
    TimerInvalidate(rec + REC_TICK_TIMER);
    BYTE *tentacle = PTR(rec, REC_CHARACTER);
    if (tentacle)
        SetTentacleAnims(tentacle, 0x93, 0x95);
    ArmGrabTimer();
}

// ---- 1. Select ----

// The pickup point "$tag-26-pickup" of the tentacle's current pose; the load-time point (+0xac) without the tag.
static void PickupPoint(BYTE *tentacle, BYTE *rec, float out[3])
{
    BYTE *inst = PTR(tentacle, CH_INSTANCE);
    int tag = GAME(FindMeshTagFn, VA_FIND_MESH_TAG)(PTR(inst, INST_CA_CHARACTER), "$tag-26-pickup");
    if (tag == -1) {
        memcpy(out, VEC(rec, REC_PICKUP), 12);
        return;
    }
    float orient[12];
    GAME(BoneWorldFn, VA_BONE_WORLD)(inst, tag, tentacle + CH_ORIENT, tentacle + CH_POS, orient, out);
}

// Distance on X/Z (y set to 0 on both, as the squeeze search does).
static float FlatDistance(const float *a, const float *b)
{
    float p[3] = {a[0], 0.0f, a[2]}, q[3] = {b[0], 0.0f, b[2]};
    return GAME(DistanceFn, VA_DISTANCE)(p, q);
}

// Grab target: the nearest grabbable candidate within 3.0 of the pickup point, else the first within reach.
static BYTE *PickGrabTarget(BYTE *tentacle, BYTE *rec, BYTE **cands, int n)
{
    float pickup[3];
    PickupPoint(tentacle, rec, pickup);
    BYTE *best = NULL;
    float bestDist = CONSTF(VA_F_NEAR);
    for (int i = 0; i < n; i++) {
        if (!Grabbable(cands[i]))
            continue;
        float d = FlatDistance(VEC(cands[i], CH_POS), pickup);
        if (d < bestDist) {
            bestDist = d;
            best = cands[i];
        }
    }
    if (best)
        return best;
    for (int i = 0; i < n; i++) {
        if (Grabbable(cands[i]) &&
            GAME(EdgeDistanceFn, VA_EDGE_DISTANCE)(tentacle, cands[i], 1, -1) < F32(rec, REC_REACH))
            return cands[i];
    }
    return NULL;
}

// Slip 4 watchdog: a grab that never reached its end (the tentacle sank or was reset during the grab move) leaves
// grab_ready 0 with the grab timer invalid. When nothing is held, thrown or being grabbed, arm the timer.
static void GrabGateWatchdog(void)
{
    if (GLOBAL(int, VA_GRAB_READY) || GAME(TimerTestFn, VA_TIMER_VALID)((void *)VA(VA_GRAB_TIMER)))
        return;
    for (int i = 0; i < NUM_TENTACLES; i++) {
        BYTE *rec = (BYTE *)VA(VA_TENTACLES) + i * REC_SIZE;
        BYTE *tentacle = PTR(rec, REC_CHARACTER);
        if (I32(rec, REC_VICTIM) != -1 || I32(rec, REC_THROWN) != -1)
            return;
        if (tentacle && GAME(AiIsModeFn, VA_AI_IS_MODE)(tentacle + CH_AI, 3, 0x35))
            return;
    }
    TRACE("trace tentacle: grab gate was stuck closed, timer armed");
    ArmGrabTimer();
}

// The grab decision, before the original ladder. Returns the target or NULL.
static BYTE *TryGrab(BYTE *tentacle, BYTE *rec, BYTE **cands, int n)
{
    GrabGateWatchdog();
    if (!GLOBAL(int, VA_GRAB_READY) || (GLOBAL(BYTE, VA_GAME_FLAGS) & 2) || !GLOBAL(DWORD, VA_MOVE_86))
        return NULL;
    if (I32(rec, REC_VICTIM) != -1 || I32(rec, REC_THROWN) != -1)
        return NULL;
    if (!(GAME(FrandFn, VA_FRAND)() < s_chance))
        return NULL;
    BYTE *target = PickGrabTarget(tentacle, rec, cands, n);
    if (!target)
        TRACE("trace tentacle %d: grab roll passed, no grabbable candidate", RecordIndex(rec));
    return target;
}

// Replaces tentacle_beast_ai_select_action (thiscall (ai), no stack arguments).
static int THISCALL TentacleSelectAction(BYTE *ai)
{
    BYTE *tentacle = PTR(ai, AI_CHARACTER);
    if (GAME(CombatBusyFn, VA_COMBAT_IS_BUSY)(tentacle + CH_COMBAT) == 1)
        return 5;
    if (GAME(MovePlayingFn, VA_MOVE_PLAYING)(tentacle, I32(ai, AI_OWN_MOVE), 0))
        return 5;
    BYTE *rec = ActiveRecord();
    U32(rec, REC_MOVE) = 0;
    void *think = rec + REC_THINK_TIMER;
    if (!GAME(TimerTestFn, VA_TIMER_VALID)(think))
        GAME(TimerRandomFn, VA_TIMER_SET_RANDOM)(think, 2500, 5000);
    if (!TimerElapsed(think))
        return 5;
    TimerInvalidate(think);

    // Candidates: the first live slot (the slots are always -1 in retail), then up to five party characters.
    BYTE *cands[5];
    int n = 0;
    for (int off = 0; off < 0x78; off += 0x18) {
        int handle = I32(rec, off);
        if (handle == -1)
            continue;
        BYTE *ch = FromHandle(handle);
        if (ch && !(U32(ch, CH_FLAGS) & FLAG_DEAD)) {
            cands[n++] = ch;
            break;
        }
    }
    BYTE *head = (BYTE *)VA(VA_LIVING_LIST);
    for (BYTE *ch = PTR(head, CH_NEXT); ch != head && n < 5; ch = PTR(ch, CH_NEXT)) {
        if (I32(ch, CH_TYPE) != 7 || (U32(ch, CH_FLAGS) & FLAG_DEAD_OR_DYING))
            continue;
        int seen = 0;
        for (int i = 0; i < n; i++)
            seen |= cands[i] == ch;
        if (!seen)
            cands[n++] = ch;
    }
    if (n == 0)
        return 4;

    BYTE *grab = TryGrab(tentacle, rec, cands, n);
    if (grab) {
        U32(rec, REC_MOVE) = GLOBAL(DWORD, VA_MOVE_86);
        I32(rec, REC_TARGET) = HandleOf(grab);
        GLOBAL(int, VA_GRAB_READY) = 0;
        TRACE("trace tentacle %d: grab pick, victim 0x%x, HP %.1f", RecordIndex(rec), HandleOf(grab), Hp(grab));
        return 3;
    }

    // The original ladder. Its grab rung (< 0.0) never fires and would fall into the swipe search.
    float r = GAME(FrandFn, VA_FRAND)();
    DWORD move;
    if (r < CONSTF(VA_F_ZERO))
        move = GLOBAL(DWORD, VA_MOVE_86);
    else if (r < CONSTF(VA_F_SQUEEZE_CHANCE))
        move = GLOBAL(DWORD, VA_MOVE_2);
    else
        move = GLOBAL(DWORD, VA_MOVE_1);
    U32(rec, REC_MOVE) = 0;

    if (move == GLOBAL(DWORD, VA_MOVE_2)) {
        float best = 1000.0f;
        for (int i = 0; i < n; i++) {
            float d = FlatDistance(VEC(cands[i], CH_POS), VEC(rec, REC_SQUEEZE));
            if (d < CONSTF(VA_F_NEAR) && d < best) {
                best = d;
                U32(rec, REC_MOVE) = GLOBAL(DWORD, VA_MOVE_2);
                I32(rec, REC_TARGET) = HandleOf(cands[i]);
            }
        }
        if (U32(rec, REC_MOVE))
            return 3;
    }

    for (int i = 0; i < n; i++) {
        if (GAME(EdgeDistanceFn, VA_EDGE_DISTANCE)(tentacle, cands[i], 1, -1) < F32(rec, REC_REACH)) {
            U32(rec, REC_MOVE) = GLOBAL(DWORD, VA_MOVE_1);
            I32(rec, REC_TARGET) = HandleOf(cands[i]);
            return 3;
        }
    }
    return 4;
}

// ---- 2. Grab ----

// Slip 9: the leadership leaves the victim only when it leads, for the first other member alive and not held.
static void MoveLeadOff(BYTE *victim)
{
    int handle = HandleOf(victim);
    BYTE *leader = GLOBAL(LPBYTE, VA_PARTY_LEADER);
    if (GLOBAL(LPBYTE, VA_CUR_CHARACTER) != victim && !(leader && I32(leader, 0xc) == handle))
        return;
    BYTE *head = (BYTE *)VA(VA_PARTY_LIST);
    for (BYTE *p = PTR(head, 0); p != head; p = PTR(p, 0)) {
        if (I32(p, 0xc) == handle)
            continue;
        BYTE *ch = FromHandle(I32(p, 0xc));
        if (ch && !(U32(ch, CH_FLAGS) & FLAG_DEAD_OR_DYING)) {
            GAME(PartySetLeaderFn, VA_PARTY_SET_LEADER)(p);
            return;
        }
    }
}

// Trigger 7 of the grab move: take the victim (0x4c68a2..0x4c69c7 with the fixes).
static void TakeVictim(BYTE *ai, BYTE *rec, BYTE *victim)
{
    BYTE *tentacle = PTR(ai, AI_CHARACTER);
    int idx = RecordIndex(rec);
    if (!victim) {
        TRACE("trace tentacle %d: grab impact, no target", idx);
        return;
    }
    if (U32(victim, CH_FLAGS) & FLAG_DEAD_OR_DYING) {
        TRACE("trace tentacle %d: grab impact, victim 0x%x dead, dying or held (flags 0x%lx)", idx, HandleOf(victim),
              (unsigned long)U32(victim, CH_FLAGS));
        return;
    }
    BYTE *flail = InfoMove(victim, 14, 0x8c);
    if (!flail || I32(flail, 8) == -1) {
        TRACE("trace tentacle %d: grab impact, victim 0x%x has no flail pose", idx, HandleOf(victim));
        return;
    }
    float dist = GAME(EdgeDistanceFn, VA_EDGE_DISTANCE)(tentacle, victim, 1, -1);
    if (!(dist < F32(rec, REC_REACH) + 1.0f)) {
        TRACE("trace tentacle %d: grab impact, victim 0x%x out of reach (%.2f, reach %.2f)", idx, HandleOf(victim), dist,
              F32(rec, REC_REACH));
        return;
    }
    BYTE *inst = PTR(victim, CH_INSTANCE);
    GAME(CharVoidFn, VA_CLEAR_STATE_ANIMS)(inst); // thiscall (character_instance *)
    GAME(SetStateWeightFn, VA_SET_STATE_WEIGHT)(inst, I32(flail, 8), 1.0f);
    I32(rec, REC_VICTIM) = HandleOf(victim);
    F32(rec, REC_RELEASE_HP) = Hp(tentacle) * CONSTF(VA_F_RELEASE_PART); // slip 7: the tentacle's HP
    TimerSet(rec + REC_HOLD_TIMER, 30000);
    TimerSet(rec + REC_TICK_TIMER, 0);
    GAME(CharIntFn, VA_SET_FLAG_2_4)(victim, -1);
    U32(victim, CH_FLAGS) |= FLAG_HELD;
    SetTentacleAnims(tentacle, 0x8d, 0x8d);
    MoveLeadOff(victim);
    TRACE("trace tentacle %d: grab, victim 0x%x, HP %.1f, tentacle HP %.1f, release below %.1f", idx, HandleOf(victim),
          Hp(victim), Hp(tentacle), F32(rec, REC_RELEASE_HP));
}

// Replaces tentacle_beast_ai_grab (thiscall (ai, victim), ret 4). victim = living_from_handle(record target), read
// by tentacle_beast_ai_frame before the select.
static void THISCALL TentacleAiGrab(BYTE *ai, BYTE *victim)
{
    BYTE *tentacle = PTR(ai, AI_CHARACTER);
    BYTE *rec = ActiveRecord();
    AddMoveFlag4();
    if (GAME(MoveFinishedFn, VA_MOVE_FINISHED)(tentacle, I32(ai, AI_OWN_MOVE))) {
        I32(ai, AI_OWN_MOVE) = -1;
        AiGoIdle(ai);
        if (I32(rec, REC_VICTIM) == -1 && I32(rec, REC_THROWN) == -1) {
            TRACE("trace tentacle %d: grab move ended with nothing held, gate opens in 10 s", RecordIndex(rec));
            ArmGrabTimer(); // slip 4
        }
        return;
    }
    if (GAME(TriggerFiredFn, VA_TRIGGER_FIRED)(tentacle, 7, 0) != 1) // slip 8: -1 is "no such trigger"
        return;
    TakeVictim(ai, rec, victim);
}

// ---- 3. Hold tick (trace only) ----

// Replaces the call of character_add_hp in the hold tick (thiscall (victim, amount, source, 1, 0), ret 0x10).
static char THISCALL TentacleTickAddHp(BYTE *victim, float amount, int source, int a, int b)
{
    float before = Hp(victim);
    char r = GAME(AddHpFn, VA_ADD_HP)(victim, amount, source, a, b);
    Log("trace tentacle %d: hold tick %+.1f, victim 0x%x, HP %.1f -> %.1f", RecordIndex(ActiveRecord()), amount,
        HandleOf(victim), before, Hp(victim));
    return r;
}

// ---- 4. Release ----

// Trigger 8 of the throw move (0x4c6ad4..0x4c6c0d with the victim test).
static void ThrowVictim(BYTE *ai, BYTE *rec)
{
    BYTE *tentacle = PTR(ai, AI_CHARACTER);
    int idx = RecordIndex(rec);
    I32(rec, REC_THROWN) = I32(rec, REC_VICTIM);
    I32(rec, REC_VICTIM) = -1;
    F32(rec, REC_GRAVITY) = 0.0f;
    BYTE *victim = FromHandle(I32(rec, REC_THROWN));
    if (!victim || IsDown(victim)) { // slip 8
        TRACE("trace tentacle %d: release, victim 0x%x %s, nothing thrown", idx, I32(rec, REC_THROWN),
              victim ? "dead" : "gone");
        if (victim)
            PutDown(victim, VEC(victim, CH_POS)[1]);
        I32(rec, REC_THROWN) = -1;
        TimerInvalidate(rec + REC_TICK_TIMER);
        SetTentacleAnims(tentacle, 0x93, 0x95);
        ArmGrabTimer();
        return;
    }
    // The original also computes the grab bone here into locals it never reads; kept for the same call sequence.
    float orient[12], bone[3];
    GAME(BoneWorldFn, VA_BONE_WORLD)(PTR(tentacle, CH_INSTANCE), I32(rec, REC_GRAB_TAG), tentacle + CH_ORIENT,
                                     tentacle + CH_POS, orient, bone);
    float k = CONSTF(VA_F_THROW_SCALE);
    for (int i = 0; i < 3; i++)
        VEC(rec, REC_VELOCITY)[i] = (VEC(rec, REC_SWIPE)[i] - VEC(victim, CH_POS)[i]) * k;
    TimerInvalidate(rec + REC_TICK_TIMER);
    SetTentacleAnims(tentacle, 0x93, 0x95);
    if (victim == GLOBAL(LPBYTE, VA_CUR_CHARACTER))
        GAME(CameraSetTargetFn, VA_CAMERA_SET_TARGET)((void *)VA(VA_GAME_CAMERA), victim);
    if (IndexOk(idx)) {
        s_throwStartY[idx] = VEC(victim, CH_POS)[1];
        s_flightTime[idx] = 0.0f;
    }
    TRACE("trace tentacle %d: throw, victim 0x%x, HP %.1f, tentacle HP %.1f, velocity (%.2f %.2f %.2f)", idx,
          HandleOf(victim), Hp(victim), Hp(tentacle), VEC(rec, REC_VELOCITY)[0], VEC(rec, REC_VELOCITY)[1],
          VEC(rec, REC_VELOCITY)[2]);
}

// Replaces tentacle_beast_ai_release (thiscall (ai), no stack arguments).
static void THISCALL TentacleAiRelease(BYTE *ai)
{
    BYTE *tentacle = PTR(ai, AI_CHARACTER);
    AddMoveFlag4();
    if (GAME(MoveFinishedFn, VA_MOVE_FINISHED)(tentacle, I32(ai, AI_OWN_MOVE))) {
        AiGoIdle(ai);
        I32(ai, AI_OWN_MOVE) = -1;
        return;
    }
    if (GAME(TriggerFiredFn, VA_TRIGGER_FIRED)(tentacle, 8, 0) != 1) // slip 8
        return;
    ThrowVictim(ai, ActiveRecord());
}

// ---- 5. Battle end ----

// Replaces tentacle_begin_sink (cdecl (record, ms)).
static void __cdecl TentacleBeginSink(BYTE *rec, int ms)
{
    BYTE *tentacle = PTR(rec, REC_CHARACTER);
    if (!tentacle)
        return;
    if (I32(rec, REC_VICTIM) != -1 || I32(rec, REC_THROWN) != -1)
        TentacleDrop(rec, "battle end / sink");
    TimerSet(rec + REC_SINK_TIMER, ms);
    U32(rec, REC_FLAGS) |= 4;
    // The party's attack goals on this tentacle (goal type 4, 0x6c) go.
    BYTE *head = (BYTE *)VA(VA_PARTY_LIST);
    for (BYTE *p = PTR(head, 0); p != head; p = PTR(p, 0)) {
        BYTE *ch = FromHandle(I32(p, 0xc));
        if (!ch)
            continue;
        BYTE *ai = ch + CH_AI;
        int goal = GAME(GoalFindFn, VA_GOAL_FIND)(ai, 4, 1, 0, 0, HandleOf(tentacle), 0, 0, 0x6c);
        if (goal != -1) {
            GAME(GoalRemoveFn, VA_GOAL_REMOVE)(ai, goal);
            AiGoIdle(ai);
        }
    }
}

// ---- 6. Frame hook ----

// The held victim, 1.5 below the grab bone with its orientation (0x4c7998..0x4c7a0c).
static void PinVictim(BYTE *tentacle, BYTE *rec, BYTE *victim)
{
    float bone[3];
    GAME(BoneWorldFn, VA_BONE_WORLD)(PTR(tentacle, CH_INSTANCE), I32(rec, REC_GRAB_TAG), tentacle + CH_ORIENT,
                                     tentacle + CH_POS, victim + CH_ORIENT, bone);
    float below = CONSTF(VA_F_HOLD_BELOW);
    for (int i = 0; i < 3; i++)
        VEC(victim, CH_POS)[i] = bone[i] - VEC(victim, CH_UP)[i] * below;
}

// One frame of the throw (0x4c7a28..0x4c7c77) with the landing fixes.
static void FlyVictim(BYTE *rec, BYTE *body)
{
    int idx = RecordIndex(rec);
    float dt = Frametime();
    float *pos = VEC(body, CH_POS), *vel = VEC(rec, REC_VELOCITY);
    LobjRay ray;
    LobjHit hit;
    memset(&ray, 0, sizeof ray);
    memset(&hit, 0, sizeof hit);
    for (int i = 0; i < 3; i++) {
        ray.start[i] = pos[i];
        ray.dir[i] = vel[i] * dt;
    }
    ray.radius = 0.01f;       // 0x3c23d70a
    ray.requireFlags = 0x40;  // floor objects
    GAME(RaycastAllFn, VA_RAYCAST_ALL)(&ray, &hit);
    int landed = 0;
    if (hit.numHits) {
        for (int i = 0; i < 3; i++)
            pos[i] = ray.start[i] + ray.dir[i] * hit.t;
        pos[1] += CONSTF(VA_F_HALF);
        landed = 1;
    } else {
        for (int i = 0; i < 3; i++)
            pos[i] += ray.dir[i];
    }

    // Slip 13: a body that falls well below where it started and where it was aimed (water), or flies too long.
    float startY = IndexOk(idx) ? s_throwStartY[idx] : pos[1];
    float aimY = VEC(rec, REC_SWIPE)[1];
    float floorY = startY < aimY ? startY : aimY;
    const char *why = "floor";
    if (!landed && pos[1] < floorY - FALL_LIMIT) {
        landed = 1;
        why = "fell below the aim point";
    }
    if (IndexOk(idx)) {
        s_flightTime[idx] += dt;
        if (!landed && s_flightTime[idx] > FLIGHT_TIMEOUT) {
            landed = 1;
            why = "flight timeout";
        }
    }

    float drag = CONSTF(VA_F_ONE) - dt * CONSTF(VA_F_RELEASE_PART);
    vel[0] *= drag;
    vel[2] *= drag;
    vel[1] -= dt * F32(rec, REC_GRAVITY);
    F32(rec, REC_GRAVITY) += dt * CONSTF(VA_F_GRAVITY);
    if (!landed)
        return;

    I32(rec, REC_THROWN) = -1;
    ArmGrabTimer(); // 0x4c7c28: the gate opens again 10 s after a landing
    PutDown(body, startY);
    TRACE("trace tentacle %d: landing (%s), victim 0x%x, HP %.1f, at (%.2f %.2f %.2f) after %.2f s", idx, why,
          HandleOf(body), Hp(body), pos[0], pos[1], pos[2], IndexOk(idx) ? s_flightTime[idx] : 0.0f);
}

// Replaces tentacle_beast_frame_hook (cdecl (tentacle)), run every frame for each tentacle.
static void __cdecl TentacleFrameHook(BYTE *tentacle)
{
    BYTE *rec = GAME(TentacleFindFn, VA_TENTACLE_FIND)(tentacle);
    if (!rec)
        return;
    if (I32(rec, REC_VICTIM) != -1) {
        BYTE *victim = FromHandle(I32(rec, REC_VICTIM));
        if (!victim || IsDown(victim))
            TentacleDrop(rec, victim ? "held victim dead" : "held victim gone"); // slip 11
        else
            PinVictim(tentacle, rec, victim);
    }
    if (I32(rec, REC_THROWN) == -1)
        return;
    BYTE *body = FromHandle(I32(rec, REC_THROWN));
    if (!body || IsDown(body)) {
        TentacleDrop(rec, body ? "thrown victim dead" : "thrown victim gone");
        return;
    }
    FlyVictim(rec, body);
}

// ---- Install ----

typedef struct {
    DWORD va;
    BYTE orig[8];
    BYTE repl[8];
    int n;
    const char *what;
} Patch;

static void SetJmp(Patch *p, void *target)
{
    DWORD rel = (DWORD)(UINT_PTR)target - ((DWORD)VA(p->va) + 5);
    p->repl[0] = 0xE9;
    memcpy(p->repl + 1, &rel, 4);
    for (int i = 5; i < p->n; i++)
        p->repl[i] = 0x90;
}

static void InstallHitBound(void)
{
    static const BYTE orig[29] = {
        0x8B, 0x97, 0x90, 0x00, 0x00, 0x00,       // mov edx, [edi+0x90]
        0x8B, 0x45, 0x00,                         // mov eax, [ebp]
        0x89, 0x84, 0x97, 0x94, 0x00, 0x00, 0x00, // mov [edi+4*edx+0x94], eax
        0x8B, 0x87, 0x90, 0x00, 0x00, 0x00,       // mov eax, [edi+0x90]
        0x40,                                     // inc eax
        0x89, 0x87, 0x90, 0x00, 0x00, 0x00};      // mov [edi+0x90], eax
    static const BYTE repl[29] = {
        0x8B, 0x97, 0x90, 0x00, 0x00, 0x00,       // mov edx, [edi+0x90]
        0x83, 0xFA, 0x04,                         // cmp edx, 4
        0x7D, 0x12,                               // jge 0x4c788d (next candidate)
        0x8B, 0x45, 0x00,                         // mov eax, [ebp]
        0x89, 0x84, 0x97, 0x94, 0x00, 0x00, 0x00, // mov [edi+4*edx+0x94], eax
        0x42,                                     // inc edx
        0x89, 0x97, 0x90, 0x00, 0x00, 0x00,       // mov [edi+0x90], edx
        0x90};
    PatchBytes(VA_HIT_STORE, orig, repl, sizeof orig, "tentacle swipe hits bounded at four slots (G251)");
}

void TentacleGrabInstall(void)
{
    s_delta = SumVa(SUM_IMAGE_BASE) - SUM_IMAGE_BASE;
    if (!GetPrivateProfileIntA("Experimental", "TentacleGrab", 0, g_cfg.iniPath)) {
        Log("tentacle grab is off ([Experimental] TentacleGrab=0)");
        return;
    }
    if (RebuiltFixInSource("TentacleGrab")) {
        // The experimental build has the grab in its source (unnamed_tentacle_beast_0x4c6480.cpp).
        Log("tentacle grab: in this exe's source (EXPERIMENTAL_BUILD), not patched");
        return;
    }
    int chance = (int)GetPrivateProfileIntA("Experimental", "TentacleGrabChance", 10, g_cfg.iniPath);
    if (chance < 0)
        chance = 0;
    if (chance > 100)
        chance = 100;
    s_chance = chance / 100.0f;
    s_trace = GetPrivateProfileIntA("Debug", "TraceTentacle", 0, g_cfg.iniPath) != 0;

    // The select goes last, so no grab can be picked before the rest is in place.
    Patch patches[] = {
        {VA_AI_GRAB, {0x53, 0x56, 0x8B, 0x35, 0xCC}, {0}, 5, "tentacle grab: guarded grab, release HP from the tentacle (G250)"},
        {VA_HOLD_TICK_PUSH, {0x68, 0x00, 0x00, 0x40, 0x40}, {0x68, 0x00, 0x00, 0x40, 0xC0}, 5,
         "tentacle grab: hold tick deals 3 instead of healing 3 (G250)"},
        {VA_AI_RELEASE, {0x83, 0xEC, 0x48, 0x56, 0x57}, {0}, 5, "tentacle grab: release with a victim test (G250)"},
        {VA_BEGIN_SINK, {0x53, 0x57, 0x8B, 0x7C, 0x24}, {0}, 5, "tentacle grab: battle end drops the victim (G250)"},
        {VA_FRAME_HOOK, {0x81, 0xEC, 0xA8, 0x00, 0x00, 0x00}, {0}, 6,
         "tentacle grab: frame hook with dead-victim drop, nav landing, flight limit (G250)"},
        {VA_SELECT_ACTION, {0x83, 0xEC, 0x34, 0x53, 0x55}, {0}, 5, "tentacle grab: selection with the grab branch (G250)"},
    };
    SetJmp(&patches[0], (void *)TentacleAiGrab);
    SetJmp(&patches[2], (void *)TentacleAiRelease);
    SetJmp(&patches[3], (void *)TentacleBeginSink);
    SetJmp(&patches[4], (void *)TentacleFrameHook);
    SetJmp(&patches[5], (void *)TentacleSelectAction);

    const int count = (int)(sizeof patches / sizeof patches[0]);
    for (int i = 0; i < count; i++) {
        if (memcmp((const void *)VA(patches[i].va), patches[i].orig, patches[i].n) != 0) {
            Log("SKIP tentacle grab: unexpected bytes at 0x%06lX, nothing patched", (unsigned long)patches[i].va);
            return;
        }
    }
    for (int i = 0; i < count; i++)
        PatchBytes(patches[i].va, patches[i].orig, patches[i].repl, patches[i].n, patches[i].what);

    if (s_trace) {
        // call character_add_hp (E8 rel32) in the hold tick -> TentacleTickAddHp, which calls it.
        BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
        DWORD site = (DWORD)VA(VA_HOLD_TICK_CALL);
        DWORD relOrig = (DWORD)VA(VA_ADD_HP) - (site + 5), relNew = (DWORD)(UINT_PTR)TentacleTickAddHp - (site + 5);
        memcpy(orig + 1, &relOrig, 4);
        memcpy(repl + 1, &relNew, 4);
        PatchBytes(VA_HOLD_TICK_CALL, orig, repl, 5, "tentacle grab: hold tick trace ([Debug] TraceTentacle)");
    }
    InstallHitBound();
    Log("tentacle grab on: chance %d%% per decision while a grab is allowed%s", chance, s_trace ? ", tracing" : "");
}
