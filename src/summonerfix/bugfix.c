// Fixes for crashes in the original game. Always on: each only changes behaviour in a case that would crash.
//
// No-sound new game crash. When DirectSound cannot start (DirectSoundCreate fails, e.g. no audio output device
// is connected), ds_init (0x55c1a0) returns before ds_init_channels and snd_stream_init, so the channel records
// (0x2f798b8, 0x2c bytes each) stay zero and the stream lock (CRITICAL_SECTION 0x2fbd998) is never initialised.
// Starting a new game then calls ds_channel_release, which reads the channel's stream index (+0xc). Zero passes
// its `test eax, eax; jl` check, so it calls EnterCriticalSection on the zeroed lock and crashes in ntdll
// (write to 0x14). Fix: the `mov eax, [esi+0xc]; test eax, eax` at 0x55c5db becomes a call to a stub that
// returns -1 (no stream) unless ds_init finished (its flag byte 0x2f8f73c is 1). With sound working nothing
// changes. ECX is free at that point (the next use reloads it).
//
// OpenGL video memory (not a crash; docs/sum-exe/game-bugs.md K64). gr_get_video_memory_mb (0x5079f0) returns a
// constant 8 for the OpenGL back end (`mov eax, 8` at 0x507a03). Its only caller, summoner_cfg_load, turns on
// low-resolution textures when summoner.cfg is missing or short and the value is <= 8 (0x43e36c..0x43e376), so a
// fresh install under SummonerFix's OpenGL renderer starts with reduced textures. Fix: report 64 instead.
//
// World sounds at the sound-effects volume squared (not a crash; game-bugs K260). snd_calc_volume_3d returns
// group volume * sound volume * attenuation. Without DirectSound3D (the default Soundcard=DirectSound), snd_play_3d
// divides only the sound volume out (fdiv [esi+0x28] at 0x521572) and hands the rest to snd_play, which multiplies
// group * sound volume in again, so the slider value counts twice (0.5 gives 0.25). Fix: the fld / fdiv / fstp at
// 0x52156e becomes a call that divides the group volume out as well, so the slider scales world sounds linearly,
// like the DirectSound3D path. At full volume nothing changes.
#include "summonerfix.h"
#include <string.h>

#define VA_DS_CHANNEL_STREAM_CHECK 0x55c5dbu // mov eax, [esi+0xc] ; test eax, eax  (8B 46 0C 85 C0)
#define VA_DS_INITED 0x2f8f73cu              // byte, set to 1 at the end of ds_init
#define VA_GL_VIDEO_MEMORY_MB 0x507a03u      // mov eax, 8 in gr_get_video_memory_mb (OpenGL case)
#define VA_SND_PLAY_3D_DIVIDE 0x52156eu      // fld [esp+0x1c] ; fdiv [esi+0x28] ; fstp [esp+0x1c] in snd_play_3d
#define VA_SND_GROUP_VOLUMES 0x2cb542cu      // float snd_group_volumes[], indexed by the sound's group

__attribute__((used)) volatile BYTE *g_dsInited;
void DsChannelStreamGuard(void);
__asm__(".text\n"
        "_DsChannelStreamGuard:\n"
        "    movl 0xc(%esi), %eax\n"
        "    movl _g_dsInited, %ecx\n"
        "    cmpb $1, (%ecx)\n"
        "    je 1f\n"
        "    orl $-1, %eax\n"
        "1:  testl %eax, %eax\n"
        "    ret\n");

__attribute__((used)) volatile float *g_sndGroupVolumes;

// (volume at [esp+0x1c] of snd_play_3d, the sound record in esi): volume / (sound volume * group volume).
// The caller already skipped this when the sound volume is 0.
__attribute__((used)) static void __cdecl SndPlay3dUnscale(float *vol, const BYTE *snd)
{
    int group = (*(const int *)(snd + 0x3c) << 5) >> 5; // the 27-bit signed group field
    float d = *(const float *)(snd + 0x28) * g_sndGroupVolumes[group];
    *vol = d != 0.0f ? *vol / d : 0.0f;
}
void SndPlay3dUnscaleStub(void);
__asm__(".text\n"
        "_SndPlay3dUnscaleStub:\n"
        "    leal 0x20(%esp), %eax\n" // snd_play_3d's [esp+0x1c] (our return address is on top)
        "    pushl %esi\n"
        "    pushl %eax\n"
        "    call _SndPlay3dUnscale\n"
        "    addl $8, %esp\n"
        "    ret\n");

void BugFixesInstall(void)
{
    g_dsInited = (volatile BYTE *)(UINT_PTR)SumVa(VA_DS_INITED);
    DWORD site = SumVa(VA_DS_CHANNEL_STREAM_CHECK);
    static const BYTE orig[5] = {0x8B, 0x46, 0x0C, 0x85, 0xC0};
    BYTE repl[5] = {0xE8};
    DWORD rel = (DWORD)(UINT_PTR)DsChannelStreamGuard - (site + 5);
    memcpy(repl + 1, &rel, 4);
    PatchBytes(VA_DS_CHANNEL_STREAM_CHECK, orig, repl, 5, "no-sound new game crash (ds_channel_release stream check)");

    static const BYTE glMemOrig[5] = {0xB8, 0x08, 0x00, 0x00, 0x00}; // mov eax, 8
    static const BYTE glMemRepl[5] = {0xB8, 0x40, 0x00, 0x00, 0x00}; // mov eax, 64
    PatchBytes(VA_GL_VIDEO_MEMORY_MB, glMemOrig, glMemRepl, 5, "OpenGL video memory reported as 8 MB (low textures on a fresh install)");

    // eax, ecx and edx are reloaded before their next use in snd_play_3d; the FPU stack is empty here.
    g_sndGroupVolumes = (volatile float *)(UINT_PTR)SumVa(VA_SND_GROUP_VOLUMES);
    static const BYTE sndOrig[11] = {0xD9, 0x44, 0x24, 0x1C, 0xD8, 0x76, 0x28, 0xD9, 0x5C, 0x24, 0x1C};
    BYTE sndRepl[11] = {0xE8, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    DWORD sndSite = SumVa(VA_SND_PLAY_3D_DIVIDE);
    DWORD sndRel = (DWORD)(UINT_PTR)SndPlay3dUnscaleStub - (sndSite + 5);
    memcpy(sndRepl + 1, &sndRel, 4);
    PatchBytes(VA_SND_PLAY_3D_DIVIDE, sndOrig, sndRepl, 11, "world sounds at the sfx volume squared without DS3D (snd_play_3d)");
}
