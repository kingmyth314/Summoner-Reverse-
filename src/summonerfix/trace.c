// Testing only (agent mode, [Debug] TraceArmourSpells=1): log the spells an armour's "+Triggered Effect:" casts.
//
// combat_trigger_reflex_ability (0x467840) rolls each equipped armour's triggered effect after a hit and, in single
// player, casts it through the one call `call spell_cast_execute` at 0x467c86 (game-bugs G300: the beneficial branch
// passes the attacker's handle as the target). The call is redirected to a stub that logs caster, spell and target
// (handle, name, team) and then continues in spell_cast_execute (0x47c5d0) with the stack unchanged.
#include "summonerfix.h"
#include <stdio.h>
#include <string.h>

#define VA_ARMOUR_SPELL_CALL 0x467c86u  // call spell_cast_execute in combat_trigger_reflex_ability
#define VA_SPELL_CAST_EXECUTE 0x47c5d0u // cdecl (Character *caster, int spell, int target handle, vector3 *pos, ...)
#define VA_LIVING_FROM_HANDLE 0x46b1a0u // cdecl (handle) -> Character * or 0

__attribute__((used)) DWORD g_spellCastExecute, g_reflexAbility;
#define VA_REFLEX_ABILITY 0x467670u // thiscall combat_trigger_reflex_ability(this = exchange: +0 attacker, +8 -> defender)
static const DWORD kReflexCalls[3] = {0x465ea5u, 0x465f26u, 0x466dddu};

static void Describe(char *out, size_t n, const BYTE *ch)
{
    if (!ch) {
        lstrcpynA(out, "(none)", (int)n);
        return;
    }
    char name[0x21];
    memcpy(name, ch + 0x2f4, 0x20); // Character +0x2f4 name
    name[0x20] = 0;
    _snprintf(out, n, "%s (handle %d, team %d)", name, *(const int *)(ch + 0x80), *(const int *)(ch + 0x98));
    out[n - 1] = 0;
}

__attribute__((used)) static void __cdecl TraceArmourSpell(const BYTE *caster, int spell, int target)
{
    typedef BYTE *(__cdecl * LivingFromHandle)(int);
    char c[96], t[96];
    Describe(c, sizeof c, caster);
    Describe(t, sizeof t, ((LivingFromHandle)(UINT_PTR)SumVa(VA_LIVING_FROM_HANDLE))(target));
    Log("armour spell: spell %d cast by %s on %s", spell, c, t);
}

// Every hit that reaches the armour roll: who hit whom (to see that the wearer is being hit at all).
__attribute__((used)) static void __cdecl TraceReflex(const BYTE *exchange)
{
    char a[96], d[96];
    const BYTE *const *defp = *(const BYTE *const *const *)(exchange + 8);
    Describe(a, sizeof a, *(const BYTE *const *)exchange);
    Describe(d, sizeof d, defp ? *defp : NULL);
    Log("hit: %s -> %s", a, d);
}
void TraceReflexStub(void);
__asm__(".text\n"
        "_TraceReflexStub:\n"
        "    pushal\n"
        "    pushl %ecx\n"
        "    call _TraceReflex\n"
        "    addl $4, %esp\n"
        "    popal\n"
        "    jmp *_g_reflexAbility\n");

void TraceArmourSpellStub(void);
__asm__(".text\n"
        "_TraceArmourSpellStub:\n"
        "    pushl 12(%esp)\n" // target (our return address is on top: caster at 4, spell at 8, target at 12)
        "    pushl 12(%esp)\n" // spell
        "    pushl 12(%esp)\n" // caster
        "    call _TraceArmourSpell\n"
        "    addl $12, %esp\n"
        "    jmp *_g_spellCastExecute\n");

// Testing only ([Debug] TraceHitVfx=1, game-bugs K20): log every combat_spawn_hit_vfx (0x4669c0) call with the
// target and its skin material (char_info +0x218), and every effect it spawns through fx_attach_to_character
// (0x48c080, called at 0x466a24 for blood and 0x466a4c for sparks). The entry's push esi ; mov esi, [esp+0xc]
// (5 bytes) becomes a jmp to a stub that logs, redoes them and jumps back to 0x4669c5.
#define VA_SPAWN_HIT_VFX 0x4669c0u
#define VA_FX_ATTACH 0x48c080u
static const DWORD kHitVfxCalls[2] = {0x466a24u, 0x466a4cu};
__attribute__((used)) DWORD g_spawnHitVfxBody, g_fxAttach;

__attribute__((used)) static void __cdecl TraceHitVfx(const BYTE *target)
{
    char t[96];
    Describe(t, sizeof t, target);
    const BYTE *info = target ? *(const BYTE *const *)(target + 0x71c) : NULL;
    Log("hit vfx: target %s, skin %d", t, info ? *(const int *)(info + 0x218) : -1);
}
__attribute__((used)) static void __cdecl TraceFxSpawn(const char *name)
{
    Log("hit vfx: spawned %s", name ? name : "(null)");
}
void TraceHitVfxStub(void);
__asm__(".text\n"
        "_TraceHitVfxStub:\n"
        "    pushal\n"
        "    pushl 40(%esp)\n" // target: 32 bytes of registers, the return address, the attacker
        "    call _TraceHitVfx\n"
        "    addl $4, %esp\n"
        "    popal\n"
        "    pushl %esi\n"
        "    movl 0xc(%esp), %esi\n"
        "    jmp *_g_spawnHitVfxBody\n");
void TraceFxSpawnStub(void);
__asm__(".text\n"
        "_TraceFxSpawnStub:\n"
        "    pushal\n"
        "    pushl 36(%esp)\n" // effect name: 32 bytes of registers, the return address
        "    call _TraceFxSpawn\n"
        "    addl $4, %esp\n"
        "    popal\n"
        "    jmp *_g_fxAttach\n");

static void TraceHitVfxInstall(void)
{
    if (!GetPrivateProfileIntA("Debug", "TraceHitVfx", 0, g_cfg.iniPath))
        return;
    g_spawnHitVfxBody = SumVa(VA_SPAWN_HIT_VFX + 5);
    g_fxAttach = SumVa(VA_FX_ATTACH);
    DWORD entry = SumVa(VA_SPAWN_HIT_VFX);
    static const BYTE eorig[5] = {0x56, 0x8B, 0x74, 0x24, 0x0C}; // push esi ; mov esi, [esp+0xc]
    BYTE erepl[5] = {0xE9};
    DWORD erel = (DWORD)(UINT_PTR)TraceHitVfxStub - (entry + 5);
    memcpy(erepl + 1, &erel, 4);
    PatchBytes(VA_SPAWN_HIT_VFX, eorig, erepl, 5, "trace combat hit effects (TraceHitVfx)");
    for (int i = 0; i < 2; i++) {
        DWORD s = SumVa(kHitVfxCalls[i]);
        BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
        DWORD ro = g_fxAttach - (s + 5), rn = (DWORD)(UINT_PTR)TraceFxSpawnStub - (s + 5);
        memcpy(orig + 1, &ro, 4);
        memcpy(repl + 1, &rn, 4);
        PatchBytes(kHitVfxCalls[i], orig, repl, 5, "trace spawned hit effects (TraceHitVfx)");
    }
}

// Testing only ([Debug] TracePhoenixInferno=1, game-bugs G215): log each Inferno hit of phoenix_special_strike.
// The call living_entity_get_hit_by_spell at 0x499962 (ecx = the Character hit, ebx = the party member the loop is
// on) becomes a call to a stub that logs both and continues in the original function.
#define VA_INFERNO_HIT_CALL 0x499962u
#define VA_GET_HIT_BY_SPELL 0x45dea0u
__attribute__((used)) DWORD g_getHitBySpell;
__attribute__((used)) static void __cdecl TraceInferno(const BYTE *hit, const BYTE *member)
{
    char a[96], b[96];
    Describe(a, sizeof a, hit);
    Describe(b, sizeof b, member);
    Log("inferno: hits %s (loop member %s)", a, b);
}
void TraceInfernoStub(void);
__asm__(".text\n"
        "_TraceInfernoStub:\n"
        "    pushal\n"
        "    pushl %ebx\n"
        "    pushl %ecx\n"
        "    call _TraceInferno\n"
        "    addl $8, %esp\n"
        "    popal\n"
        "    jmp *_g_getHitBySpell\n");

static void TraceInfernoInstall(void)
{
    if (!GetPrivateProfileIntA("Debug", "TracePhoenixInferno", 0, g_cfg.iniPath))
        return;
    g_getHitBySpell = SumVa(VA_GET_HIT_BY_SPELL);
    DWORD s = SumVa(VA_INFERNO_HIT_CALL);
    BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
    DWORD ro = g_getHitBySpell - (s + 5), rn = (DWORD)(UINT_PTR)TraceInfernoStub - (s + 5);
    memcpy(orig + 1, &ro, 4);
    memcpy(repl + 1, &rn, 4);
    PatchBytes(VA_INFERNO_HIT_CALL, orig, repl, 5, "trace Phoenix Inferno hits (TracePhoenixInferno)");
}

// Testing only ([Debug] LuminarBeamTargetGone=1, game-bugs C250): while the Luminar's beam charges (AI sub-mode
// 0x32), luminar_ai_frame's target lookup (the call living_from_handle at 0x4c95fc, esi = ai_state) returns NULL,
// as when the target dies or a summon disappears during the beam. Without the C250 fix the impact frame crashes.
#define VA_LUMINAR_TARGET_LOOKUP 0x4c95fcu
#define VA_LIVING_FROM_HANDLE2 0x46b1a0u
__attribute__((used)) DWORD g_livingFromHandle;
__attribute__((used)) volatile DWORD g_luminarBeamFrames; // frames with the target forced gone (agent peek)
void LuminarTargetGoneStub(void);
__asm__(".text\n"
        "_LuminarTargetGoneStub:\n"
        "    cmpl $0x32, 4(%esi)\n" // ai_state +4 sub-mode: 0x32 = the beam
        "    jne 1f\n"
        "    incl _g_luminarBeamFrames\n"
        "    xorl %eax, %eax\n"
        "    ret\n"
        "1:  jmp *_g_livingFromHandle\n");

static void LuminarTestInstall(void)
{
    if (!GetPrivateProfileIntA("Debug", "LuminarBeamTargetGone", 0, g_cfg.iniPath))
        return;
    g_livingFromHandle = SumVa(VA_LIVING_FROM_HANDLE2);
    DWORD s = SumVa(VA_LUMINAR_TARGET_LOOKUP);
    BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
    DWORD ro = g_livingFromHandle - (s + 5), rn = (DWORD)(UINT_PTR)LuminarTargetGoneStub - (s + 5);
    memcpy(orig + 1, &ro, 4);
    memcpy(repl + 1, &rn, 4);
    if (PatchBytes(VA_LUMINAR_TARGET_LOOKUP, orig, repl, 5, "test: Luminar beam target gone (LuminarBeamTargetGone)"))
        Log("LuminarBeamTargetGone: beam frame counter at %p", (void *)&g_luminarBeamFrames);
}

void TraceInstall(void)
{
    TraceHitVfxInstall();
    LuminarTestInstall();
    TraceInfernoInstall();
    if (!GetPrivateProfileIntA("Debug", "TraceArmourSpells", 0, g_cfg.iniPath))
        return;
    g_spellCastExecute = SumVa(VA_SPELL_CAST_EXECUTE);
    DWORD site = SumVa(VA_ARMOUR_SPELL_CALL);
    BYTE orig[5] = {0xE8}, repl[5] = {0xE8};
    DWORD relOrig = g_spellCastExecute - (site + 5);
    DWORD relNew = (DWORD)(UINT_PTR)TraceArmourSpellStub - (site + 5);
    memcpy(orig + 1, &relOrig, 4);
    memcpy(repl + 1, &relNew, 4);
    PatchBytes(VA_ARMOUR_SPELL_CALL, orig, repl, 5, "trace armour triggered spells (TraceArmourSpells)");
    g_reflexAbility = SumVa(VA_REFLEX_ABILITY);
    for (int i = 0; i < 3; i++) {
        DWORD s = SumVa(kReflexCalls[i]);
        DWORD ro = g_reflexAbility - (s + 5), rn = (DWORD)(UINT_PTR)TraceReflexStub - (s + 5);
        memcpy(orig + 1, &ro, 4);
        memcpy(repl + 1, &rn, 4);
        PatchBytes(kReflexCalls[i], orig, repl, 5, "trace hits that reach the armour roll");
    }
}

// Testing only ([Debug] TraceGlLights=1, game-bugs K310): count the faces that take the unclipped lightmap path of
// levelrender_draw_list_opengl, whose three colour loads at 0x491f9e / 0x491fb5 / 0x491fcd all read the red float of
// levelrender_gl_vert_colors (0x24268f8, 12-byte r, g, b records, index in ebx). The first load (6 bytes) becomes a
// call to a stub that counts the face, notes when green or blue differs from red (a coloured light that the bug
// turns grey), keeps one sample, and then does the original fld. Read the counts with the agent command "k310".
#define VA_GL_UNCLIPPED_COLOR_LOAD 0x491f9eu // fld dword ptr [ebx + 0x24268f8]
#define VA_GL_VERT_COLORS 0x24268f8u
__attribute__((used)) DWORD g_glVertColors;
__attribute__((used)) volatile LONG g_k310Faces, g_k310Coloured;
__attribute__((used)) volatile float g_k310Sample[3];
void GlLightTraceStub(void);
__asm__(".text\n"
        "_GlLightTraceStub:\n"
        "    pushl %eax\n"
        "    pushl %edx\n"
        "    movl _g_glVertColors, %eax\n"
        "    addl %ebx, %eax\n"
        "    incl _g_k310Faces\n"
        "    movl (%eax), %edx\n"
        "    cmpl 4(%eax), %edx\n"
        "    jne 1f\n"
        "    cmpl 8(%eax), %edx\n"
        "    je 2f\n"
        "1:  incl _g_k310Coloured\n"
        "    movl %edx, _g_k310Sample\n"
        "    movl 4(%eax), %edx\n"
        "    movl %edx, _g_k310Sample+4\n"
        "    movl 8(%eax), %edx\n"
        "    movl %edx, _g_k310Sample+8\n"
        "2:  flds (%eax)\n"
        "    popl %edx\n"
        "    popl %eax\n"
        "    ret\n");

// Also count calls of levelrender_draw_list_opengl (0x491500) and those whose third argument, the number of point
// lights near the drawn object, is non-zero: its first instruction mov al, [0x23c5b38] (5 bytes) becomes a call.
#define VA_GL_DRAW_LIST_ENTRY 0x491500u
#define VA_GL_DRAW_LIST_GUARD 0x23c5b38u
__attribute__((used)) DWORD g_glDrawListGuard;
__attribute__((used)) volatile LONG g_k310Calls, g_k310CallsLit, g_k310LightsMax;
void GlDrawListTraceStub(void);
__asm__(".text\n"
        "_GlDrawListTraceStub:\n"
        "    incl _g_k310Calls\n"
        "    movl 16(%esp), %eax\n" // third argument: our return at 0, the caller's return at 4, args at 8, 12, 16
        "    testl %eax, %eax\n"
        "    je 1f\n"
        "    incl _g_k310CallsLit\n"
        "    cmpl _g_k310LightsMax, %eax\n"
        "    jle 1f\n"
        "    movl %eax, _g_k310LightsMax\n"
        "1:  movl _g_glDrawListGuard, %eax\n"
        "    movb (%eax), %al\n"
        "    ret\n");

void TraceGlDrawStats(long *calls, long *lit, long *maxLights)
{
    *calls = g_k310Calls;
    *lit = g_k310CallsLit;
    *maxLights = g_k310LightsMax;
}

void TraceGlLightStats(long *faces, long *coloured, float rgb[3])
{
    *faces = g_k310Faces;
    *coloured = g_k310Coloured;
    rgb[0] = g_k310Sample[0];
    rgb[1] = g_k310Sample[1];
    rgb[2] = g_k310Sample[2];
}

void TraceGlLightInstall(void)
{
    if (!GetPrivateProfileIntA("Debug", "TraceGlLights", 0, g_cfg.iniPath))
        return;
    g_glVertColors = SumVa(VA_GL_VERT_COLORS);
    DWORD site = SumVa(VA_GL_UNCLIPPED_COLOR_LOAD);
    static const BYTE orig[6] = {0xD9, 0x83, 0xF8, 0x68, 0x42, 0x02};
    BYTE repl[6] = {0xE8, 0, 0, 0, 0, 0x90};
    DWORD rel = (DWORD)(UINT_PTR)GlLightTraceStub - (site + 5);
    memcpy(repl + 1, &rel, 4);
    PatchBytes(VA_GL_UNCLIPPED_COLOR_LOAD, orig, repl, 6, "count unclipped lightmap colour loads (TraceGlLights)");

    g_glDrawListGuard = SumVa(VA_GL_DRAW_LIST_GUARD);
    DWORD entry = SumVa(VA_GL_DRAW_LIST_ENTRY);
    static const BYTE eorig[5] = {0xA0, 0x38, 0x5B, 0x3C, 0x02}; // mov al, [0x23c5b38]
    BYTE erepl[5] = {0xE8};
    DWORD erel = (DWORD)(UINT_PTR)GlDrawListTraceStub - (entry + 5);
    memcpy(erepl + 1, &erel, 4);
    PatchBytes(VA_GL_DRAW_LIST_ENTRY, eorig, erepl, 5, "count draw-list calls with point lights (TraceGlLights)");
}
