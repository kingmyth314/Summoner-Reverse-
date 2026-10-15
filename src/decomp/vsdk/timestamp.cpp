// timestamp.cpp - Sum.exe v1.40, code 0x5017f0-0x501b16
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact / likely / unsure (see frametime.cpp header).

#include <stdlib.h>
#include "timestamp.h"

// vsdk\vfile (0x525990, 0x526250), __thiscall
class File {
public:
    int read_int(int min_version, int default_value);
    void write_int(int value);
};

// ---- .data ---------------------------------------------------------------------------------
int Timestamp_ticker = 1;           // 0x5a5758  only this file uses it
int Timestamp_realtime_ticker = 1;  // 0x5a575c  only this file uses it

// ---- .bss ----------------------------------------------------------------------------------
static int Timestamp_paused;        // 0x257f0ac

// 0x5017f0  Confidence: likely (the "room" local may instead be written out twice; same code).
// Called once per frame by FrameDone with the frame's milliseconds.
void UpdateTicker(int ms)
{
    if (!Timestamp_paused) {
        int room = MAX_TIME - Timestamp_ticker;
        if (ms > room)
            Timestamp_ticker = ms - room;
        else
            Timestamp_ticker += ms;
    }

    int room = MAX_TIME - Timestamp_realtime_ticker;
    if (ms > room)
        Timestamp_realtime_ticker = ms - room;
    else
        Timestamp_realtime_ticker += ms;
}

// 0x501840  Confidence: exact.
void IncrementPauseCounter()
{
    Timestamp_paused++;
}

// 0x501850  Confidence: exact.
void DecrementPauseCounter()
{
    Timestamp_paused--;
}

// ==== GameTimer (Timestamp_ticker) ==========================================================

// 0x501860  Confidence: exact (constructor returns this in EAX).
GameTimer::GameTimer()
{
    value = -1;
}

// 0x501870  Confidence: exact.
void GameTimer::reset()
{
    value = -1;
}

// 0x501880  Confidence: likely. Negative timeouts are allowed and wrap below 0.
void GameTimer::SetTimeout(int ms)
{
    if (ms >= 0) {
        int room = MAX_TIME - Timestamp_ticker;
        if (ms > room)
            value = ms - room;
        else
            value = Timestamp_ticker + ms;
    } else {
        value = Timestamp_ticker + ms;   // stored, then tested and stored again
        if (value < 0)
            value += MAX_TIME;
    }
}

// 0x5018d0  Confidence: exact. rand() is called before the arguments are loaded.
void GameTimer::SetRandomTimeout(int min_ms, int max_ms)
{
    SetTimeout(rand() % (max_ms - min_ms + 1) + min_ms);
}

// 0x501900  Confidence: exact.
void GameTimer::invalidate()
{
    value = -1;
}

// 0x501910  Confidence: exact. Invalid timers never elapse; half the wrap range decides the side.
bool GameTimer::elapsed()
{
    if (value < 0)
        return false;
    if (value > Timestamp_ticker)
        return value - Timestamp_ticker > HALF_MAX_TIME;
    return Timestamp_ticker - value <= HALF_MAX_TIME;
}

// 0x501940  Confidence: likely (VC6 CSEs the differences; the final "return value - ticker" is
// emitted as NEG of ticker - value).
int GameTimer::GetRemainingTime()
{
    if (value < 0)
        return MAX_TIME;
    if (value > Timestamp_ticker) {
        if (value - Timestamp_ticker > HALF_MAX_TIME)
            return value - Timestamp_ticker - MAX_TIME;
        return value - Timestamp_ticker;
    }
    if (Timestamp_ticker - value > HALF_MAX_TIME)
        return value - Timestamp_ticker + MAX_TIME;
    return value - Timestamp_ticker;
}

// 0x501980  Confidence: exact (a call, not inlined).
int GameTimer::GetElapsedTime()
{
    return -GetRemainingTime();
}

// 0x501990  Confidence: exact.
bool GameTimer::valid()
{
    return value >= 0;
}

// 0x5019a0  Confidence: exact. Savegame load: the stored remaining time becomes a new timeout.
void GameTimer::read(File *f)
{
    SetTimeout(f->read_int(0, 0));
}

// 0x5019c0  Confidence: exact. GetRemainingTime is called before f is loaded into ECX.
void GameTimer::write(File *f)
{
    f->write_int(GetRemainingTime());
}

// ==== RealtimeTimer (Timestamp_realtime_ticker) =============================================

// 0x5019e0  Confidence: exact.
RealtimeTimer::RealtimeTimer()
{
    value = -1;
}

// 0x5019f0  Confidence: exact.
void RealtimeTimer::reset()
{
    value = -1;
}

// 0x501a00  Confidence: likely (same shape as GameTimer::SetTimeout).
void RealtimeTimer::SetTimeout(int ms)
{
    if (ms >= 0) {
        int room = MAX_TIME - Timestamp_realtime_ticker;
        if (ms > room)
            value = ms - room;
        else
            value = Timestamp_realtime_ticker + ms;
    } else {
        value = Timestamp_realtime_ticker + ms;
        if (value < 0)
            value += MAX_TIME;
    }
}

// 0x501a50  Confidence: exact.
void RealtimeTimer::SetRandomTimeout(int min_ms, int max_ms)
{
    SetTimeout(rand() % (max_ms - min_ms + 1) + min_ms);
}

// 0x501a80  Confidence: exact.
void RealtimeTimer::invalidate()
{
    value = -1;
}

// 0x501a90  Confidence: exact.
bool RealtimeTimer::elapsed()
{
    if (value < 0)
        return false;
    if (value > Timestamp_realtime_ticker)
        return value - Timestamp_realtime_ticker > HALF_MAX_TIME;
    return Timestamp_realtime_ticker - value <= HALF_MAX_TIME;
}

// 0x501ac0  Confidence: likely.
int RealtimeTimer::GetRemainingTime()
{
    if (value < 0)
        return MAX_TIME;
    if (value > Timestamp_realtime_ticker) {
        if (value - Timestamp_realtime_ticker > HALF_MAX_TIME)
            return value - Timestamp_realtime_ticker - MAX_TIME;
        return value - Timestamp_realtime_ticker;
    }
    if (Timestamp_realtime_ticker - value > HALF_MAX_TIME)
        return value - Timestamp_realtime_ticker + MAX_TIME;
    return value - Timestamp_realtime_ticker;
}

// 0x501b00  Confidence: exact.
int RealtimeTimer::GetElapsedTime()
{
    return -GetRemainingTime();
}

// 0x501b10  Confidence: exact.
bool RealtimeTimer::valid()
{
    return value >= 0;
}
