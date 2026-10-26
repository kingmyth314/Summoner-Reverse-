// gamma.cpp - brightness and the fast-mode badge in the source (include/modern/modern.h; L3 player build). Not
// original source.
//
// SummonerFix's gamma.c and its speed badge (display.c) for a build that runs without it. Both are drawn from
// modern_swap_buffers, after the bars and before a capture, as SummonerFix's SwapBuffers hook does.
//
// Brightness (game-bugs K90): the options screen's slider calls gr_set_gamma, which stores gr_gamma and builds
// gr_gamma_ramp[256] (the "gamma" console command only stores Summoner_options.gamma_correction; the options
// screen's apply and GameInit pass it on). Only the Direct3D and Glide back ends use them; under OpenGL the
// slider does nothing. [Display] Brightness:
//   Auto (default)  the shader pass, else the monitor ramp
//   Shader          the finished frame is copied into a texture and drawn back through pow(c, 1/gamma), the curve
//                   gr_set_gamma builds (OpenGL 2.0); only the game window changes
//   Ramp            SetDeviceGammaRamp from gr_gamma_ramp: free, but it changes the whole monitor, so the saved ramp
//                   is put back while the game window is not in front and at exit
//   Off
// At gamma 1.0 (the default) nothing is drawn.
//
// The badge: "FAST xN" in the picture's top-left corner while the speed key's fast mode is on in play.
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/gr/gr.h"
#include "modern/modern.h"
#include "summoner/summoner.h"

#if defined(MODERN_BUILD)

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#define GL_RGB8_ 0x8051
#define GL_ACTIVE_TEXTURE_ 0x84E0
#define GL_TEXTURE0_ 0x84C0
#define GL_TEXTURE1_ 0x84C1
#define GL_CURRENT_PROGRAM_ 0x8B8D
#define GL_FRAGMENT_SHADER_ 0x8B30
#define GL_COMPILE_STATUS_ 0x8B81
#define GL_LINK_STATUS_ 0x8B82

// The OpenGL 2.0 functions, through wglGetProcAddress.
static struct {
    void (WINAPI *ActiveTexture)(GLenum);
    void (WINAPI *UseProgram)(GLuint);
    GLuint (WINAPI *CreateShader)(GLenum);
    void (WINAPI *ShaderSource)(GLuint, GLsizei, const char *const *, const GLint *);
    void (WINAPI *CompileShader)(GLuint);
    void (WINAPI *GetShaderiv)(GLuint, GLenum, GLint *);
    GLuint (WINAPI *CreateProgram)();
    void (WINAPI *AttachShader)(GLuint, GLuint);
    void (WINAPI *LinkProgram)(GLuint);
    void (WINAPI *GetProgramiv)(GLuint, GLenum, GLint *);
    GLint (WINAPI *GetUniformLocation)(GLuint, const char *);
    void (WINAPI *Uniform1f)(GLint, GLfloat);
    void (WINAPI *Uniform1i)(GLint, GLint);
} gl2;

enum { MODE_AUTO, MODE_SHADER, MODE_RAMP, MODE_OFF };
static int bright_mode = -1;     // read once, at the first frame
static int shader_state;         // 0 not tried, 1 ready, -1 unavailable
static GLuint shader_prog, shader_tex;
static int tex_w, tex_h, loc_tex, loc_exp;

static bool ramp_saved, ramp_active;
static WORD saved_ramp[3][256];
static float ramp_gamma;

static const char *fragment_src =
    "uniform sampler2D tex;\n"
    "uniform float e;\n"
    "void main() {\n"
    "    vec3 c = texture2D(tex, gl_TexCoord[0].xy).rgb;\n"
    "    gl_FragColor = vec4(pow(c, vec3(e)), 1.0);\n"
    "}\n";

static bool shader_init()
{
    typedef PROC(WINAPI * gpa_fn)(LPCSTR);
    gpa_fn gpa = (gpa_fn)(void *)GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglGetProcAddress");
    if (!gpa)
        return false;
#define GL2(f, n) *(PROC *)&gl2.f = gpa(n)
    GL2(ActiveTexture, "glActiveTexture"); GL2(UseProgram, "glUseProgram"); GL2(CreateShader, "glCreateShader");
    GL2(ShaderSource, "glShaderSource"); GL2(CompileShader, "glCompileShader"); GL2(GetShaderiv, "glGetShaderiv");
    GL2(CreateProgram, "glCreateProgram"); GL2(AttachShader, "glAttachShader"); GL2(LinkProgram, "glLinkProgram");
    GL2(GetProgramiv, "glGetProgramiv"); GL2(GetUniformLocation, "glGetUniformLocation");
    GL2(Uniform1f, "glUniform1f"); GL2(Uniform1i, "glUniform1i");
#undef GL2
    void **p = (void **)&gl2;
    for (size_t i = 0; i < sizeof gl2 / sizeof *p; i++)
        if (!p[i]) {
            modern_log("brightness: OpenGL 2.0 functions missing, using the monitor gamma ramp");
            return false;
        }
    GLuint fs = gl2.CreateShader(GL_FRAGMENT_SHADER_);
    gl2.ShaderSource(fs, 1, &fragment_src, NULL);
    gl2.CompileShader(fs);
    GLint ok = 0;
    gl2.GetShaderiv(fs, GL_COMPILE_STATUS_, &ok);
    if (!ok) {
        modern_log("brightness: shader compile failed, using the monitor gamma ramp");
        return false;
    }
    shader_prog = gl2.CreateProgram();
    gl2.AttachShader(shader_prog, fs);
    gl2.LinkProgram(shader_prog);
    gl2.GetProgramiv(shader_prog, GL_LINK_STATUS_, &ok);
    if (!ok) {
        modern_log("brightness: shader link failed, using the monitor gamma ramp");
        return false;
    }
    loc_tex = gl2.GetUniformLocation(shader_prog, "tex");
    loc_exp = gl2.GetUniformLocation(shader_prog, "e");
    glGenTextures(1, &shader_tex);
    modern_log("brightness: OpenGL shader pass ready");
    return true;
}

static void shader_pass(int cw, int ch, float gamma)
{
    GLint active_unit = GL_TEXTURE0_, bound = 0, prog = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE_, &active_unit);
    glGetIntegerv(GL_CURRENT_PROGRAM_, &prog);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    gl2.ActiveTexture(GL_TEXTURE1_);
    glDisable(GL_TEXTURE_2D);
    gl2.ActiveTexture(GL_TEXTURE0_);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_ALPHA_TEST); glDisable(GL_FOG);
    glDisable(GL_LIGHTING); glDisable(GL_SCISSOR_TEST); glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, shader_tex);
    if (cw != tex_w || ch != tex_h) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8_, cw, ch, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        tex_w = cw;
        tex_h = ch;
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cw, ch);
    glViewport(0, 0, cw, ch);
    glMatrixMode(GL_TEXTURE); glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    gl2.UseProgram(shader_prog);
    gl2.Uniform1i(loc_tex, 0);
    gl2.Uniform1f(loc_exp, 1.0f / gamma);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(1, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(0, 1);
    glEnd();
    gl2.UseProgram((GLuint)prog);
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_TEXTURE); glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, (GLuint)bound);
    glPopAttrib();   // viewport, enables, colour, matrix mode
    gl2.ActiveTexture((GLenum)active_unit);
}

static void ramp_restore()
{
    if (!ramp_active)
        return;
    HDC dc = GetDC(NULL);
    SetDeviceGammaRamp(dc, saved_ramp);
    ReleaseDC(NULL, dc);
    ramp_active = false;
}

static void ramp_apply(HWND wnd, float gamma)
{
    if (GetForegroundWindow() != wnd) {   // never leave the desktop bright or dark behind the game
        ramp_restore();
        return;
    }
    if (ramp_active && gamma == ramp_gamma)
        return;
    HDC dc = GetDC(NULL);
    if (!ramp_saved)
        ramp_saved = GetDeviceGammaRamp(dc, saved_ramp) != 0;
    if (ramp_saved) {
        static WORD ramp[3][256];
        for (int i = 0; i < 256; i++) {
            int v = gr_gamma_ramp[i] < 0 ? 0 : gr_gamma_ramp[i] > 255 ? 255 : gr_gamma_ramp[i];
            ramp[0][i] = ramp[1][i] = ramp[2][i] = (WORD)(v << 8 | v);
        }
        if (SetDeviceGammaRamp(dc, ramp)) {
            ramp_active = true;
            ramp_gamma = gamma;
        } else {
            static bool logged;
            if (!logged) {
                modern_log("brightness: SetDeviceGammaRamp refused gamma %.2f (the driver limits the ramp)", (double)gamma);
                logged = true;
            }
        }
    }
    ReleaseDC(NULL, dc);
}

static void ramp_at_exit()
{
    ramp_restore();
}

void modern_brightness(HWND wnd, int cw, int ch)
{
    if (bright_mode < 0) {
        char mode[16];
        modern_ini_str("Display", "Brightness", "Auto", mode, sizeof mode);
        bright_mode = !_stricmp(mode, "Off") ? MODE_OFF : !_stricmp(mode, "Ramp") ? MODE_RAMP
                    : !_stricmp(mode, "Shader") ? MODE_SHADER : MODE_AUTO;
        atexit(ramp_at_exit);
    }
    float gamma = gr_gamma;
    if (bright_mode == MODE_OFF || !(gamma > 0.0f) || fabsf(gamma - 1.0f) < 0.005f) {
        ramp_restore();
        return;
    }
    if (bright_mode != MODE_RAMP && shader_state == 0)
        shader_state = shader_init() ? 1 : -1;
    if (bright_mode != MODE_RAMP && shader_state == 1) {
        ramp_restore();
        shader_pass(cw, ch, gamma);
        return;
    }
    if (bright_mode == MODE_SHADER)
        return;   // Brightness=Shader: no monitor ramp
    ramp_apply(wnd, gamma);
}

// ---- the fast-mode badge ----

static int font_height;
static GLuint font_base;

void modern_speed_badge(HDC dc, int cw, int ch, int off_x, int off_y, double scale)
{
    float factor = modern_keys_speed_active();
    if (factor <= 0.0f || gameseq_state_cur != 4)
        return;
    int fh = (int)(18 * scale + 0.5);
    if (fh != font_height) {   // (re)build the bitmap font for this output size
        HFONT font = CreateFontA(-fh, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 ANTIALIASED_QUALITY, DEFAULT_PITCH, "Arial");
        HGDIOBJ old = SelectObject(dc, font);
        if (!font_base)
            font_base = glGenLists(96);
        wglUseFontBitmapsA(dc, 32, 96, font_base);
        SelectObject(dc, old);
        DeleteObject(font);
        font_height = fh;
    }
    char text[32];
    _snprintf(text, sizeof text, "FAST x%g", (double)factor);
    text[sizeof text - 1] = 0;
    int len = (int)strlen(text);

    glPushAttrib(GL_ALL_ATTRIB_BITS);   // viewport, enables, colour, lists
    if (shader_state == 1) {   // gl2 loaded: the second texture unit off as well
        gl2.ActiveTexture(GL_TEXTURE1_);
        glDisable(GL_TEXTURE_2D);
        gl2.ActiveTexture(GL_TEXTURE0_);
    }
    glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_FOG);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_ALPHA_TEST); glDisable(GL_CULL_FACE);
    glViewport(0, 0, cw, ch);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, cw, 0, ch, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    // In the top-left of the game picture (GL's origin is bottom-left).
    float pad = 6 * (float)scale, x0 = (float)off_x + 12 * (float)scale;
    float y1 = (float)(ch - off_y) - 12 * (float)scale, w = len * fh * 0.62f + 2 * pad, h = fh + 2 * pad;
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(x0, y1 - h); glVertex2f(x0 + w, y1 - h); glVertex2f(x0 + w, y1); glVertex2f(x0, y1);
    glEnd();
    glColor4f(1.0f, 0.82f, 0.25f, 1.0f);   // gold, like the game's highlight text
    glRasterPos2i((int)(x0 + pad), (int)(y1 - pad - fh * 0.8f));
    glListBase(font_base - 32);
    glCallLists(len, GL_UNSIGNED_BYTE, text);
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glPopAttrib();
}

#endif
