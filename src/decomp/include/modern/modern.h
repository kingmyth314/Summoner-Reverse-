// modern.h - the in-source host (docs/future/modern-port.md, rung L3b). Not original source.
//
// What SummonerFix gives a rebuilt exe, for when it is not there: a 64-bit exe (SummonerFix is a 32-bit DLL), or a
// 32-bit clang build run without it. Compiled only into MODERN_BUILD (the clang builds; src/decomp/modern/*.cpp).
// Every service stands down when SummonerFix is loaded (its dinput.dll beside the exe), so the 32-bit clang build
// keeps SummonerFix's behaviour for comparison.
//
//   host.cpp     settings (summonerfix.ini beside the exe, the same keys), the address table (<exe>.addr: Sum.exe
//                address -> this exe's, for agent commands that take Sum.exe addresses), the save folder, the log
//   display.cpp  the output window (borderless full screen or a window of the configured size), the game's 4:3 picture
//                scaled into it (glViewport), the bars cleared, the mouse mapped back to game pixels, captures
//   agent.cpp    agent mode ([Debug] Agent=1): SummonerFix's agent protocol and commands (tools/agent.mjs)
#ifndef MODERN_MODERN_H
#define MODERN_MODERN_H

#ifdef MODERN_BUILD
#include <windows.h>
#include <stdarg.h>

#ifndef _WIN64
// The 32-bit clang build uses VC6's stdarg.h, whose va_start takes the address of the last named parameter; clang
// does not keep that parameter where the macro expects (agent mode's output came out empty). clang's builtins give
// the same x86 va_list (a char pointer) that VC6's _vsnprintf reads.
#undef va_start
#undef va_end
#define va_start(ap, v) __builtin_va_start(ap, v)
#define va_end(ap) __builtin_va_end(ap)
#endif

// ---- host.cpp ----
void modern_host_init();                       // WinMain, first: settings, SummonerFix detection, address table, log
bool modern_host();                            // the services are on (SummonerFix is not loaded)
int  modern_ini_int(const char *section, const char *key, int def);
void modern_ini_str(const char *section, const char *key, const char *def, char *out, int size);
const char *modern_dir();                      // the exe's folder, no trailing backslash
const char *modern_ini_path();                 // summonerfix.ini beside the exe
const char *modern_save_folder();              // "savetest" with [Debug] TestSaves=1, else NULL (the original's)
void *modern_sum_va(unsigned int sum_addr);    // a Sum.exe address in this exe (the address table), NULL if unknown
void modern_log(const char *fmt, ...);         // summonerfix.log beside the exe (the test tools read it)

// ---- display.cpp ----
void modern_window_setup(HWND hwnd);           // gr_init's windowed branch: the output window
bool modern_window_activate(HWND hwnd, bool active);   // os_wndproc's WM_ACTIVATE: true = handled
void modern_gl_viewport(int x, int y, int w, int h);
// Game pixels -> window pixels (client size, scale, offset of the game's (0, 0), the 1024-wide menu centred); false
// before the window exists or while SummonerFix is loaded.
bool modern_display_map(int *cw, int *ch, double *scale, int *off_x, int *off_y, bool *menu_shift);
void modern_gl_read_pixels(int x, int y, int w, int h, unsigned format, unsigned type, void *data);
BOOL modern_swap_buffers(HDC dc);              // bars, capture, then SwapBuffers
BOOL modern_screen_to_client(HWND hwnd, POINT *p);    // the mouse poll: window pixels -> game pixels
BOOL modern_client_to_screen(HWND hwnd, POINT *p);    // the mouse warp: game pixels -> screen
void modern_request_capture(const char *path);
bool modern_capture_pending();
bool modern_capture_wanted();                  // a capture is requested (the Vulkan back end takes it)
void modern_capture_bgra(const unsigned char *bgra, int w, int h);   // the window's pixels, top row first
void modern_set_agent_cursor(int client_x, int client_y);
void modern_get_agent_cursor(int *client_x, int *client_y);

// ---- keys.cpp ----
void modern_keys_init();                       // after os_init: [KeyBindings], [ConsoleKeys], the [Game] hotkeys
int  modern_key_bind(int code);                // key_mark, after the layout swap: the bound game key
int  modern_key_physical(int code);            // key_to_ascii: the physical key that produced a game key
void modern_mouse_button(int button, bool down);   // mouse_msg_handler: a button through its binding
void modern_key_mark_raw(int code, int down);  // a game key code, not remapped (agent mode, the controller)
void modern_keys_agent_speed(bool on);         // agent mode's speedkey: as if the speed key were pressed
float modern_keys_speed_active();              // the speed factor while fast mode is on, else 0

// ---- registry.cpp ----
// vsdk/registry.cpp's registry calls: SoftwareVolitionSummoner is answered from summonerfix.ini, as SummonerFix does.
LONG WINAPI modern_RegOpenKeyExA(HKEY h, LPCSTR sub, DWORD opt, REGSAM sam, PHKEY out);
LONG WINAPI modern_RegCreateKeyExA(HKEY h, LPCSTR sub, DWORD res, LPSTR cls, DWORD opt, REGSAM sam,
                                   LPSECURITY_ATTRIBUTES sa, PHKEY out, LPDWORD disp);
LONG WINAPI modern_RegQueryValueExA(HKEY h, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD cb);
LONG WINAPI modern_RegSetValueExA(HKEY h, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD cb);
LONG WINAPI modern_RegDeleteValueA(HKEY h, LPCSTR name);
LONG WINAPI modern_RegDeleteKeyA(HKEY h, LPCSTR sub);
LONG WINAPI modern_RegCloseKey(HKEY h);

// ---- gamma.cpp ----
void modern_brightness(HWND wnd, int cw, int ch);   // modern_swap_buffers: the options screen's brightness (K90)
void modern_speed_badge(HDC dc, int cw, int ch, int off_x, int off_y, double scale);   // "FAST xN" in play

// ---- agent.cpp ----
void modern_agent_init();                      // after the main window exists (os_init)
#endif

#endif
