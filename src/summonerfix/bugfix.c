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
#include "summonerfix.h"
#include <string.h>

#define VA_DS_CHANNEL_STREAM_CHECK 0x55c5dbu // mov eax, [esi+0xc] ; test eax, eax  (8B 46 0C 85 C0)
#define VA_DS_INITED 0x2f8f73cu              // byte, set to 1 at the end of ds_init
#define VA_GL_VIDEO_MEMORY_MB 0x507a03u      // mov eax, 8 in gr_get_video_memory_mb (OpenGL case)

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
}
