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

void TraceInstall(void)
{
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
}
