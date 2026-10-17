// unnamed_movement_0x531d20.cpp - Sum.exe v1.40, code 0x531d20-0x532353. Written from the Sum.exe
// disassembly (tools/asm.mjs; the floats 0x581930 = 0.0001, 0x580e04 = 0.75, 0x57e990 = 1.0 read from the
// image). Target: VC++ 6.0 (cl 12.00.8966), /O2 with the inline members called out of line (the vector3
// constructors and operators are calls to their COMDAT copies, as /Ob0 compiles them). Confidence per
// function: exact (control flow, calls, constants and globals match), likely (one detail of statement order
// or expression form is a guess), unsure (the shape itself is a guess).
//
// The free-movement (slew) controller of the debug camera: the number pad and A / Z move, the keypad turns,
// shift is faster and ctrl slower (console "movement_vel_change"), and both are smoothed exponentially.
// FILE NAME NOT KNOWN (D5315): no path string, no strings.tbl block, no reference name; a placeholder. The
// unit is cut by its static initialiser run (table entries 0x5889a8-0x5889c0) after debug.cpp and ends
// before norlsallocpool.cpp (0x532360). .bss 0x2cbff30-0x2cbffa4 (between the os object's 0x2cbff24 and
// timer.cpp's 0x2cbffa8); .data 0x5a98d4 (Allow_movement_vel_change) and the literals 0x5a98d8-0x5a99bd
// (not pooled: "movement_vel_change" three times).
// sum.symbols names: movement_velocity (0x2cbff68) is the velocity the keys ask for and movement_rotvel
// (0x2cbff98) the smoothed linear velocity, not a rotation (see NOTES); the names are kept.
//
// Header: summoner/unnamed_input_0x4343a0.h declares movement_init / movement_read_controls (other units).

#include <math.h>
#include <stdio.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/console.h"
#include "vsdk/os/key.h"
#include "vsdk/ui/ui.h"
#include "summoner/unnamed_input_0x4343a0.h"

void smooth_approach_exp(float tau, float target, float current, float dt, float *out_current,
                         float *out_delta);
void cmd_movement_vel_change();

// ---- .data -----------------------------------------------------------------------------------------------
bool Allow_movement_vel_change = true;               // 0x5a98d4

// ---- .bss (0x2cbff30-0x2cbffa4) --------------------------------------------------------------------------
float movement_rot_tau;                              // 0x2cbff30 movement_init's fourth argument (name ours)
// The static initialisers, in table order (each a jmp to `mov ecx, obj / jmp vector3::vector3`):
// 0x531d20  Confidence: exact. init_movement_vec_2cbff58 (table 0x5889a8).
vector3 movement_max_vel;                            // 0x2cbff58
// 0x531d40  Confidence: exact. init_movement_vec_2cbff88 (0x5889ac).
vector3 movement_max_rotvel;                         // 0x2cbff88
// 0x531d60  Confidence: exact. init_movement_vec_2cbff98 (0x5889b0).
vector3 movement_rotvel;                             // 0x2cbff98 the smoothed velocity
// 0x531d80  Confidence: exact. init_movement_vec_2cbff38 (0x5889b4).
vector3 movement_turn_rate;                          // 0x2cbff38 the smoothed turn rate (name ours)
// 0x531da0  Confidence: exact. init_movement_vec_2cbff68 (0x5889b8).
vector3 movement_velocity;                           // 0x2cbff68 the velocity the keys ask for
// 0x531dc0  Confidence: exact. init_movement_vec_2cbff48 (0x5889bc).
vector3 movement_turn_target;                        // 0x2cbff48 the turn rate the keys ask for (name ours)
// 0x531de0  Confidence: exact. init_ccmd_movement_vel_change_thunk (0x5889c0): jmp 0x531df0.
// 0x531df0  Confidence: exact. init_ccmd_movement_vel_change.
console_command ccmd_movement_vel_change("movement_vel_change", "Toggles Allow_movement_vel_change",
                                         cmd_movement_vel_change);   // 0x2cbff78
float movement_tau;                                  // 0x2cbff84 movement_init's third argument (name ours)
bool movement_inited;                                // 0x2cbffa4 set by movement_init, not read (name ours)

// 0x531e10  Confidence: exact. The console's bool command.
void cmd_movement_vel_change()
{
    if (calling_a_command_function) {
        process_command(0xc1);
        if (arg_type & ARG_TRUE)
            Allow_movement_vel_change = true;
        else if (arg_type & ARG_FALSE)
            Allow_movement_vel_change = false;
        else if (arg_type & ARG_NONE)
            Allow_movement_vel_change ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "movement_vel_change", "Allow_movement_vel_change");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "movement_vel_change", Allow_movement_vel_change ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x531ed0  Confidence: exact (the order of the six limit stores is scheduled). (maximum speed, maximum turn
// rate, the two smoothing times): the input unit (0x4343a0) passes 10, 1, 0.1, 0.1.
void movement_init(float max_vel, float max_rotvel, float a3, float a4)
{
    movement_max_vel.x = max_vel;
    movement_max_vel.y = max_vel;
    movement_max_vel.z = max_vel;
    movement_max_rotvel.x = max_rotvel;
    movement_max_rotvel.y = max_rotvel;
    movement_max_rotvel.z = max_rotvel;
    movement_rotvel.zero();
    movement_velocity.zero();
    movement_turn_rate.zero();
    movement_turn_target.zero();
    movement_tau = a3;
    movement_rot_tau = a4;
    movement_inited = true;
}

// 0x531f50  Confidence: likely (the key differences as written; the x87 code keeps each difference in st(1)
// and multiplies the limit loaded after it). Nothing while a text box (gadget type 6) has the focus. The keys
// (pad 3 / pad 1, pad + / pad Enter, A / Z) give the wanted velocity in the view's axes, the pad 8 / 2, 6 / 4,
// 7 / 9 the turn rates (roll at 3/4); both approach their wanted values with movement_tau / movement_rot_tau,
// the position moves by the integral and the orientation turns by rate * dt (angles p = x, b = z, h = y).
void movement_read_controls(vector3 *pos, matrix *orient, float dt)
{
    vector3 max_vel;
    vector3 max_rotvel;
    vector3 v;
    vector3 delta;

    if (ui_current_gadget && ui_current_gadget->get_type() == 6)
        return;
    max_vel = movement_max_vel;
    max_rotvel = movement_max_rotvel;
    if (Allow_movement_vel_change) {
        if (key_is_down(KEY_RSHIFT) | key_is_down(KEY_LSHIFT)) {
            max_vel *= 5.0f;
            max_rotvel *= 5.0f;
        }
        if (key_is_down(KEY_RCONTROL) | key_is_down(KEY_LCONTROL)) {
            max_vel *= 0.1f;
            max_rotvel *= 0.25f;
        }
    }
    v.x = (key_down_time(0x51) - key_down_time(0x4f)) * max_vel.x;
    v.y = (key_down_time(0x4e) - key_down_time(0x9c)) * max_vel.y;
    v.z = (key_down_time(0x1e) - key_down_time(0x2c)) * max_vel.z;
    movement_velocity = orient->rvec * v.x;
    movement_velocity += orient->uvec * v.y;
    movement_velocity += orient->fvec * v.z;
    smooth_approach_exp(movement_tau, movement_velocity.x, movement_rotvel.x, dt, &movement_rotvel.x, &delta.x);
    smooth_approach_exp(movement_tau, movement_velocity.y, movement_rotvel.y, dt, &movement_rotvel.y, &delta.y);
    smooth_approach_exp(movement_tau, movement_velocity.z, movement_rotvel.z, dt, &movement_rotvel.z, &delta.z);
    *pos = *pos + delta;

    movement_turn_target.x = (key_down_time(0x48) - key_down_time(0x50)) * max_rotvel.x;
    movement_turn_target.y = (key_down_time(0x4d) - key_down_time(0x4b)) * max_rotvel.y;
    movement_turn_target.z = (key_down_time(0x47) - key_down_time(0x49)) * max_rotvel.z * 0.75f;
    smooth_approach_exp(movement_rot_tau, movement_turn_target.x, movement_turn_rate.x, dt, &movement_turn_rate.x, NULL);
    smooth_approach_exp(movement_rot_tau, movement_turn_target.y, movement_turn_rate.y, dt, &movement_turn_rate.y, NULL);
    smooth_approach_exp(movement_rot_tau, movement_turn_target.z, movement_turn_rate.z, dt, &movement_turn_rate.z, NULL);
    matrix rot(movement_turn_rate.x * dt, movement_turn_rate.z * dt, movement_turn_rate.y * dt);
    *orient = *orient * rot;
    orient->orthogonalize();
}

// 0x5322c0  Confidence: exact (exp is the inline x87 intrinsic, f2xm1 / fscale). (time constant, wanted
// value, current value, dt, out new value, out integral over dt): new = wanted + (current - wanted) *
// exp(-dt / tau), integral = wanted * dt + (current - wanted) * (1 - exp(-dt / tau)) * tau; with tau below
// 0.0001 the value jumps to the wanted one. Either output may be NULL. (sum.symbols lists the second and
// third arguments the other way round, see NOTES.)
void smooth_approach_exp(float tau, float target, float current, float dt, float *out_current,
                         float *out_delta)
{
    if (tau < 0.0001f) {
        if (out_delta)
            *out_delta = target * dt;
        if (out_current)
            *out_current = target;
        return;
    }
    float diff = current - target;
    float e = (float)exp(-(dt / tau));
    if (out_delta)
        *out_delta = (1.0f - e) * diff * tau + target * dt;
    if (out_current)
        *out_current = e * diff + target;
}
