// Controller (gamepad) inputs and their default game keys, as [Controller] keys in summonerfix.ini.
// Shared by SummonerFix (controller.c) and SummonerFixConfig. Header-only: include in one file per program.
#pragma once

// Bit masks of the inputs in one 32-bit "down" word: the low 16 bits are XINPUT_GAMEPAD wButtons bits; the
// triggers and the right stick's four directions get bits above them.
#define PAD_DPAD_UP 0x0001u
#define PAD_DPAD_DOWN 0x0002u
#define PAD_DPAD_LEFT 0x0004u
#define PAD_DPAD_RIGHT 0x0008u
#define PAD_START 0x0010u
#define PAD_BACK 0x0020u
#define PAD_LTHUMB 0x0040u
#define PAD_RTHUMB 0x0080u
#define PAD_LSHOULDER 0x0100u
#define PAD_RSHOULDER 0x0200u
#define PAD_A 0x1000u
#define PAD_B 0x2000u
#define PAD_X 0x4000u
#define PAD_Y 0x8000u
#define PAD_LTRIGGER 0x10000u
#define PAD_RTRIGGER 0x20000u
#define PAD_CAM_LEFT 0x40000u  // right stick pushed left
#define PAD_CAM_RIGHT 0x80000u
#define PAD_CAM_UP 0x100000u
#define PAD_CAM_DOWN 0x200000u

typedef struct {
    const char *iniKey; // [Controller] key
    unsigned mask;
    const char *def;    // default key name (keynames.h), or "none"
    const char *label;  // what the default does (docs/sum-exe/controls.md)
} PadInput;

// Keys are game keys (sent to key_mark as the agent mode's key command does, so [KeyBindings] do not apply);
// MouseLeft/MouseRight/MouseMiddle press the game's mouse buttons.
static const PadInput k_padInputs[] = {
    {"A", PAD_A, "MouseLeft", "move / attack / talk"},
    {"B", PAD_B, "MouseRight", "chain attack"},
    {"X", PAD_X, "X", "interact with the nearest object"},
    {"Y", PAD_Y, "I", "inventory"},
    {"LB", PAD_LSHOULDER, "Tab", "next party member (with LT: previous)"},
    {"RB", PAD_RSHOULDER, "H", "solo mode on/off"},
    {"LT", PAD_LTRIGGER, "LShift", "Shift (with B: chain slot 1)"},
    {"RT", PAD_RTRIGGER, "Space", "pause"},
    {"Start", PAD_START, "Esc", "play menu"},
    {"Back", PAD_BACK, "M", "automap"},
    {"DPadUp", PAD_DPAD_UP, "1", "quick slot 1"},
    {"DPadRight", PAD_DPAD_RIGHT, "2", "quick slot 2"},
    {"DPadDown", PAD_DPAD_DOWN, "3", "quick slot 3"},
    {"DPadLeft", PAD_DPAD_LEFT, "4", "quick slot 4"},
    {"LS", PAD_LTHUMB, "C", "spells panel"},
    {"RS", PAD_RTHUMB, "F12", "camera mode"},
    {"CameraLeft", PAD_CAM_LEFT, "Left", "turn the camera (right stick)"},
    {"CameraRight", PAD_CAM_RIGHT, "Right", "turn the camera (right stick)"},
    {"CameraUp", PAD_CAM_UP, "Up", "zoom in (right stick)"},
    {"CameraDown", PAD_CAM_DOWN, "Down", "zoom out (right stick)"},
};
#define PAD_INPUT_COUNT (int)(sizeof k_padInputs / sizeof k_padInputs[0])

// Numeric [Controller] settings and their defaults.
#define PAD_DEF_DEADZONE 7849       // left stick, radial (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
#define PAD_DEF_CURSOR_SPEED 800    // game pixels per second at full deflection (the UI is 1024x768)
#define PAD_DEF_CAMERA_THRESHOLD 16000 // right stick deflection that holds a camera key
#define PAD_DEF_TRIGGER_THRESHOLD 30   // 0-255 (XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
