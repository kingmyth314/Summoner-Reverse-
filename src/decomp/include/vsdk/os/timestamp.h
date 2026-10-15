// timestamp.h - Sum.exe v1.40 millisecond tickers and timers (timestamp.cpp, 0x5017f0-0x501b16)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

// Both tickers wrap at MAX_TIME (1,072,800,000 ms, about 12.4 days).
#define MAX_TIME 0x3ff1a100
#define HALF_MAX_TIME (MAX_TIME / 2) // 0x1ff8d080

class File; // vsdk\vfile

extern int Timestamp_ticker;           // 0x5a5758  game-time ms, starts at 1, frozen while paused
extern int Timestamp_realtime_ticker;  // 0x5a575c  real-time ms, starts at 1

void UpdateTicker(int ms);
void IncrementPauseCounter();
void DecrementPauseCounter();

// One dword: the ticker value at which the timer expires, -1 = invalid.
// All members are out of line (__thiscall); /Ob1 builds inline nothing that is not marked inline.
class GameTimer {
public:
    GameTimer();
    void reset();
    void SetTimeout(int ms);
    void SetRandomTimeout(int min_ms, int max_ms);
    void invalidate();
    bool elapsed();
    int GetRemainingTime();
    int GetElapsedTime();
    bool valid();
    void read(File *f);
    void write(File *f);

    int value;
};

class RealtimeTimer {
public:
    RealtimeTimer();
    void reset();
    void SetTimeout(int ms);
    void SetRandomTimeout(int min_ms, int max_ms);
    void invalidate();
    bool elapsed();
    int GetRemainingTime();
    int GetElapsedTime();
    bool valid();
    // no read/write: the RealtimeTimer block ends at RealtimeTimer__valid (0x501b10)

    int value;
};

#endif
