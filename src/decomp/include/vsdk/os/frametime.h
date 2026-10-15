// frametime.h - Sum.exe v1.40 frame timer (frametime.cpp, 0x501470-0x5017e6)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef FRAMETIME_H
#define FRAMETIME_H

// .data (initialised), 0x5a573c-0x5a5757
extern float actual_frametime;        // 0x5a573c  measured seconds, before clamping
extern float reported_frametime;      // 0x5a5740  seconds used by gameplay (80+ readers)
extern float fps;                     // 0x5a5744  16-frame average
extern float frametime_divisor;       // 0x5a5748  always 1.0
extern float min_reported_frametime;  // 0x5a574c  1/120
extern float max_reported_frametime;  // 0x5a5750  0.25

// .bss
extern int milliseconds_unpaused;     // 0x257f08c  read by bm_get_current_frame
extern int cutscene_active_count;     // 0x257f090  label is ours; used only by cutscene/game code
extern float cutscene_frametime;      // 0x257f094  label is ours; used only by cutscene/game code

void frametime_initialize();
void FrameDone();
void frametime_set_divisor(float divisor);
float frametime_get_divisor();
void frametime_set_limits(float a, float b);
void frametime_get_limits(float *min_out, float *max_out);
void frametime_unlimit();
void frametime_restore_limits();
void increase_frametime_pause_counter();
void decrease_frametime_pause_counter();
void increase_animation_pause_counter();
void decrease_animation_pause_counter();

#endif
