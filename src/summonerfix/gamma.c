// In-game brightness (gamma) for the OpenGL renderer (game-bugs K90).
//
// The options screen's gamma slider and the "gamma" console command call gr_set_gamma (0x506a90), which stores
// gr_gamma (0x5a6064) and builds gr_gamma_ramp[256] (0x25d8e0c, 255 * (i/255)^(1/gamma)). Only the Direct3D back
// end (a hardware gamma ramp) and Glide (grGammaCorrectionValue) use it; under OpenGL the slider does nothing.
// We apply it from the SwapBuffers hook, before the speed badge and the bars are final:
//   - gamma 1.0 (the default): nothing at all;
//   - Shader (OpenGL 2.0, any graphics card of the last 15 years): copy the finished frame into a texture and draw
//     it back through pow(c, 1/gamma), the curve gr_set_gamma builds. One full-screen copy and quad per frame;
//     only the game window changes;
//   - Ramp (fallback, or Brightness=Ramp): SetDeviceGammaRamp from gr_gamma_ramp. Free, but it changes the whole
//     monitor, so the saved ramp is put back while the game window is not in front and when the game exits.
// [Display] Brightness = Auto (shader, else ramp) | Shader | Ramp | Off.
#include "summonerfix.h"
#include <math.h>

#define VA_GR_GAMMA 0x5a6064u       // float gr_gamma
#define VA_GR_GAMMA_RAMP 0x25d8e0cu // int gr_gamma_ramp[256], values 0..255

#define GL_TEXTURE_2D 0x0DE1
#define GL_RGB 0x1907
#define GL_RGB8 0x8051
#define GL_UNSIGNED_BYTE 0x1401
#define GL_NEAREST 0x2600
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ALL_ATTRIB_BITS 0x000FFFFF
#define GL_PROJECTION 0x1701
#define GL_MODELVIEW 0x1700
#define GL_TEXTURE 0x1702
#define GL_QUADS 7

typedef void(WINAPI *PFN_v_u)(unsigned);
typedef void(WINAPI *PFN_v_v)(void);
typedef void(WINAPI *PFN_v_ui)(unsigned, int);
typedef void(WINAPI *PFN_v_uu)(unsigned, unsigned);
typedef void(WINAPI *PFN_v_ff)(float, float);
typedef void(WINAPI *PFN_v_ffff)(float, float, float, float);
typedef void(WINAPI *PFN_v_iiii)(int, int, int, int);
typedef void(WINAPI *PFN_v_dddddd)(double, double, double, double, double, double);
typedef void(WINAPI *PFN_v_iu)(int, unsigned *);
typedef void(WINAPI *PFN_v_uip)(unsigned, int *);
typedef void(WINAPI *PFN_TexImage2D)(unsigned, int, int, int, int, int, unsigned, unsigned, const void *);
typedef void(WINAPI *PFN_TexParameteri)(unsigned, unsigned, int);
typedef void(WINAPI *PFN_CopyTexSubImage2D)(unsigned, int, int, int, int, int, int, int);
typedef unsigned(WINAPI *PFN_CreateShader)(unsigned);
typedef void(WINAPI *PFN_ShaderSource)(unsigned, int, const char **, const int *);
typedef unsigned(WINAPI *PFN_CreateProgram)(void);
typedef void(WINAPI *PFN_GetIv)(unsigned, unsigned, int *);
typedef int(WINAPI *PFN_GetUniformLocation)(unsigned, const char *);
typedef void(WINAPI *PFN_Uniform1f)(int, float);
typedef void(WINAPI *PFN_Uniform1i)(int, int);

static struct {
    PFN_v_u PushAttrib, MatrixMode, Enable, Disable, Begin;
    PFN_v_v PopAttrib, PushMatrix, PopMatrix, LoadIdentity, End;
    PFN_v_ff TexCoord2f, Vertex2f;
    PFN_v_ffff Color4f;
    PFN_v_iiii Viewport;
    PFN_v_dddddd Ortho;
    PFN_v_iu GenTextures;
    PFN_v_uu BindTexture;
    PFN_v_uip GetIntegerv;
    PFN_TexImage2D TexImage2D;
    PFN_TexParameteri TexParameteri;
    PFN_CopyTexSubImage2D CopyTexSubImage2D;
    PFN_v_u ActiveTexture, UseProgram, CompileShader, LinkProgram;
    PFN_v_uu AttachShader;
    PFN_CreateShader CreateShader;
    PFN_ShaderSource ShaderSource;
    PFN_CreateProgram CreateProgram;
    PFN_GetIv GetShaderiv, GetProgramiv;
    PFN_GetUniformLocation GetUniformLocation;
    PFN_Uniform1f Uniform1f;
    PFN_Uniform1i Uniform1i;
} gl;

enum { MODE_AUTO, MODE_SHADER, MODE_RAMP, MODE_OFF };
static int g_mode = MODE_AUTO;
static int g_shaderState; // 0 not tried, 1 ready, -1 unavailable
static unsigned g_prog, g_tex;
static int g_texW, g_texH, g_locTex, g_locExp;

static BOOL g_rampSaved, g_rampActive;
static WORD g_savedRamp[3][256];
static float g_rampGamma;

static const char *kFragment =
    "uniform sampler2D tex;\n"
    "uniform float e;\n"
    "void main() {\n"
    "    vec3 c = texture2D(tex, gl_TexCoord[0].xy).rgb;\n"
    "    gl_FragColor = vec4(pow(c, vec3(e)), 1.0);\n"
    "}\n";

void GammaConfigure(const char *mode)
{
    if (!lstrcmpiA(mode, "Off"))
        g_mode = MODE_OFF;
    else if (!lstrcmpiA(mode, "Ramp"))
        g_mode = MODE_RAMP;
    else if (!lstrcmpiA(mode, "Shader"))
        g_mode = MODE_SHADER;
    else
        g_mode = MODE_AUTO;
}

static BOOL ShaderInit(void)
{
    HMODULE m = GetModuleHandleA("opengl32.dll");
    typedef PROC(WINAPI * PFN_wglGetProcAddress)(LPCSTR);
    PFN_wglGetProcAddress gpa = m ? (PFN_wglGetProcAddress)GetProcAddress(m, "wglGetProcAddress") : NULL;
    if (!gpa)
        return FALSE;
#define GL1(f, n) *(FARPROC *)&gl.f = GetProcAddress(m, n)
#define GL2(f, n) *(FARPROC *)&gl.f = (FARPROC)gpa(n)
    GL1(PushAttrib, "glPushAttrib"); GL1(PopAttrib, "glPopAttrib"); GL1(MatrixMode, "glMatrixMode");
    GL1(PushMatrix, "glPushMatrix"); GL1(PopMatrix, "glPopMatrix"); GL1(LoadIdentity, "glLoadIdentity");
    GL1(Enable, "glEnable"); GL1(Disable, "glDisable"); GL1(Begin, "glBegin"); GL1(End, "glEnd");
    GL1(TexCoord2f, "glTexCoord2f"); GL1(Vertex2f, "glVertex2f"); GL1(Color4f, "glColor4f");
    GL1(Viewport, "glViewport"); GL1(Ortho, "glOrtho"); GL1(GenTextures, "glGenTextures");
    GL1(BindTexture, "glBindTexture"); GL1(GetIntegerv, "glGetIntegerv"); GL1(TexImage2D, "glTexImage2D");
    GL1(TexParameteri, "glTexParameteri"); GL1(CopyTexSubImage2D, "glCopyTexSubImage2D");
    GL2(ActiveTexture, "glActiveTexture"); GL2(UseProgram, "glUseProgram"); GL2(CompileShader, "glCompileShader");
    GL2(LinkProgram, "glLinkProgram"); GL2(AttachShader, "glAttachShader"); GL2(CreateShader, "glCreateShader");
    GL2(ShaderSource, "glShaderSource"); GL2(CreateProgram, "glCreateProgram"); GL2(GetShaderiv, "glGetShaderiv");
    GL2(GetProgramiv, "glGetProgramiv"); GL2(GetUniformLocation, "glGetUniformLocation");
    GL2(Uniform1f, "glUniform1f"); GL2(Uniform1i, "glUniform1i");
#undef GL1
#undef GL2
    void **p = (void **)&gl;
    for (size_t i = 0; i < sizeof gl / sizeof *p; i++)
        if (!p[i]) {
            Log("brightness: OpenGL 2.0 functions missing, using the monitor gamma ramp");
            return FALSE;
        }
    unsigned fs = gl.CreateShader(GL_FRAGMENT_SHADER);
    gl.ShaderSource(fs, 1, &kFragment, NULL);
    gl.CompileShader(fs);
    int ok = 0;
    gl.GetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        Log("brightness: shader compile failed, using the monitor gamma ramp");
        return FALSE;
    }
    g_prog = gl.CreateProgram();
    gl.AttachShader(g_prog, fs);
    gl.LinkProgram(g_prog);
    gl.GetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        Log("brightness: shader link failed, using the monitor gamma ramp");
        return FALSE;
    }
    g_locTex = gl.GetUniformLocation(g_prog, "tex");
    g_locExp = gl.GetUniformLocation(g_prog, "e");
    gl.GenTextures(1, &g_tex);
    Log("brightness: OpenGL shader pass ready");
    return TRUE;
}

static void ShaderPass(int cw, int ch, float gamma)
{
    int activeUnit = GL_TEXTURE0, bound = 0, prog = 0;
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &activeUnit);
    gl.GetIntegerv(GL_CURRENT_PROGRAM, &prog);
    gl.PushAttrib(GL_ALL_ATTRIB_BITS);
    gl.ActiveTexture(GL_TEXTURE1);
    gl.Disable(GL_TEXTURE_2D);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    gl.Disable(0x0B71 /* DEPTH_TEST */); gl.Disable(0x0BE2 /* BLEND */); gl.Disable(0x0BC0 /* ALPHA_TEST */);
    gl.Disable(0x0B60 /* FOG */); gl.Disable(0x0B50 /* LIGHTING */); gl.Disable(0x0C11 /* SCISSOR_TEST */);
    gl.Disable(0x0B44 /* CULL_FACE */);
    gl.Enable(GL_TEXTURE_2D);
    gl.BindTexture(GL_TEXTURE_2D, g_tex);
    if (cw != g_texW || ch != g_texH) {
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cw, ch, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        g_texW = cw;
        g_texH = ch;
    }
    gl.CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cw, ch);
    gl.Viewport(0, 0, cw, ch);
    gl.MatrixMode(GL_TEXTURE); gl.PushMatrix(); gl.LoadIdentity();
    gl.MatrixMode(GL_PROJECTION); gl.PushMatrix(); gl.LoadIdentity();
    gl.Ortho(0, 1, 0, 1, -1, 1);
    gl.MatrixMode(GL_MODELVIEW); gl.PushMatrix(); gl.LoadIdentity();
    gl.UseProgram(g_prog);
    gl.Uniform1i(g_locTex, 0);
    gl.Uniform1f(g_locExp, 1.0f / gamma);
    gl.Color4f(1, 1, 1, 1);
    gl.Begin(GL_QUADS);
    gl.TexCoord2f(0, 0); gl.Vertex2f(0, 0);
    gl.TexCoord2f(1, 0); gl.Vertex2f(1, 0);
    gl.TexCoord2f(1, 1); gl.Vertex2f(1, 1);
    gl.TexCoord2f(0, 1); gl.Vertex2f(0, 1);
    gl.End();
    gl.UseProgram((unsigned)prog);
    gl.MatrixMode(GL_MODELVIEW); gl.PopMatrix();
    gl.MatrixMode(GL_PROJECTION); gl.PopMatrix();
    gl.MatrixMode(GL_TEXTURE); gl.PopMatrix();
    gl.BindTexture(GL_TEXTURE_2D, (unsigned)bound);
    gl.PopAttrib(); // viewport, enables, colour, matrix mode
    gl.ActiveTexture((unsigned)activeUnit);
}

static void RampRestore(void)
{
    if (!g_rampActive)
        return;
    HDC dc = GetDC(NULL);
    SetDeviceGammaRamp(dc, g_savedRamp);
    ReleaseDC(NULL, dc);
    g_rampActive = FALSE;
}

static void RampApply(HWND wnd, float gamma)
{
    if (GetForegroundWindow() != wnd) { // never leave the desktop bright or dark behind the game
        RampRestore();
        return;
    }
    if (g_rampActive && gamma == g_rampGamma)
        return;
    HDC dc = GetDC(NULL);
    if (!g_rampSaved)
        g_rampSaved = GetDeviceGammaRamp(dc, g_savedRamp);
    if (g_rampSaved) {
        static WORD ramp[3][256];
        const volatile int *src = (const volatile int *)(UINT_PTR)SumVa(VA_GR_GAMMA_RAMP);
        for (int i = 0; i < 256; i++) {
            int v = src[i] < 0 ? 0 : src[i] > 255 ? 255 : src[i];
            ramp[0][i] = ramp[1][i] = ramp[2][i] = (WORD)(v << 8 | v);
        }
        if (SetDeviceGammaRamp(dc, ramp)) {
            g_rampActive = TRUE;
            g_rampGamma = gamma;
        } else {
            static BOOL logged;
            if (!logged) {
                Log("brightness: SetDeviceGammaRamp refused gamma %.2f (the driver limits the ramp)", gamma);
                logged = TRUE;
            }
        }
    }
    ReleaseDC(NULL, dc);
}

// Called from the SwapBuffers hook with the window's client size, before the speed badge.
void GammaApply(HWND wnd, int cw, int ch)
{
    float gamma = *(volatile float *)(UINT_PTR)SumVa(VA_GR_GAMMA);
    if (g_mode == MODE_OFF || !(gamma > 0.0f) || fabsf(gamma - 1.0f) < 0.005f) {
        RampRestore();
        return;
    }
    if (g_mode != MODE_RAMP && g_shaderState == 0)
        g_shaderState = ShaderInit() ? 1 : -1;
    if (g_mode != MODE_RAMP && g_shaderState == 1) {
        RampRestore();
        ShaderPass(cw, ch, gamma);
        return;
    }
    if (g_mode == MODE_SHADER)
        return; // Brightness=Shader: no monitor ramp
    RampApply(wnd, gamma);
}

void GammaShutdown(void)
{
    RampRestore();
}
