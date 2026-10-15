// vsdk/os/timer.h - the high-resolution timer (timer.cpp in sum.symbols, 0x530bf0-0x5329b1; directory
// guessed). Not original source.
#ifndef VSDK_OS_TIMER_H
#define VSDK_OS_TIMER_H

void timer_wait(int ms);              // 0x530bf0 Sleep wrapper
int  timer_get(int scale);            // 0x5328d0 elapsed QPC ticks * scale / frequency (32-bit result)

#endif
