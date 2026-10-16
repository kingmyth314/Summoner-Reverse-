// unnamed_time_format_0x532660.cpp - Sum.exe v1.40, code 0x532660-0x532816. Written from the Sum.exe
// disassembly (tools/asm.mjs; the floats 0x58070c = 1/3600 and 0x581e58 = 3600 read from the image). Target:
// VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact (control flow, calls, constants and globals
// match), likely (one detail of statement order or expression form is a guess), unsure (the shape itself is
// a guess).
//
// "h:mm:ss" / "m:ss" from a number of seconds; no caller in v1.40 (dead code).
// FILE NAME NOT KNOWN (D5316): it lies between norlsallocpool.cpp (ends 0x532653) and timer.cpp (0x532820);
// its literals 0x5a99c0-0x5a99d6 follow the free-movement unit's and are not pooled (three "%d:", two "0"),
// timer.cpp has none, so nothing ties it to timer.cpp; a placeholder unit of its own.

#include <stdio.h>
#include <string.h>

void time_format_hms(int seconds, char *buf);

// 0x532660  Confidence: likely (the hours test is `> 0` twice and `!= 0` once in the code; the temporary
// buffer's size, 16, from the frame). The hours part is "%d:" and the minutes get a leading zero only after
// hours; the seconds always get one.
void time_format_hms(int seconds, char *buf)
{
    char part[16];
    float t = (float)seconds;
    int hours = (int)(t * (1.0f / 3600.0f));
    int minutes;
    int secs;

    if (hours > 0)
        t -= hours * 3600.0f;
    secs = (int)t;
    minutes = secs / 60;
    secs = secs % 60;
    if (hours > 0) {
        sprintf(buf, "%d:", hours);
        if (minutes < 10) {
            strcpy(part, "0");
            strcat(buf, part);
        }
    }
    if (hours) {
        sprintf(part, "%d:", minutes);
        strcat(buf, part);
    } else {
        sprintf(buf, "%d:", minutes);
    }
    if (secs < 10) {
        strcpy(part, "0");
        strcat(buf, part);
    }
    sprintf(part, "%d", secs);
    strcat(buf, part);
}
