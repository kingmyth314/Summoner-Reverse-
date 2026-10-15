// vsdk/gr/opengl/gl_min.h - the part of the OpenGL 1.1 / GLU API that Sum.exe imports (ours; the original
// unit included the SDK's <GL/gl.h> and <GL/glu.h>, which this tree does not use). Not original source.
//
// Linkage as the binary shows it: every gl* call in gr_opengl.cpp is `call dword ptr [IAT]`
// (OPENGL32.dll imports 0x57e25c-0x57e320), so they are declared dllimport like gl.h's WINGDIAPI; the two
// GLU calls go through the linker's jmp thunks 0x57b5c4 / 0x57b5ca (GLU32.dll imports 0x57e058 /
// 0x57e05c), as glu.h declares them without dllimport. The wgl* functions, ChoosePixelFormat,
// SetPixelFormat, DescribePixelFormat and SwapBuffers come from <windows.h> (wingdi.h).
#ifndef VSDK_GR_OPENGL_GL_MIN_H
#define VSDK_GR_OPENGL_GL_MIN_H

#include <windows.h>

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef void           GLvoid;

// ---- constants (values as pushed in Sum.exe) ------------------------------------------------------
#define GL_ZERO                         0
#define GL_ONE                          1
#define GL_LINE_STRIP                   0x0003
#define GL_TRIANGLES                    0x0004
#define GL_POLYGON                      0x0009
#define GL_DEPTH_BUFFER_BIT             0x00000100
#define GL_EQUAL                        0x0202
#define GL_LEQUAL                       0x0203
#define GL_GREATER                      0x0204
#define GL_ONE_MINUS_SRC_COLOR          0x0301
#define GL_SRC_ALPHA                    0x0302
#define GL_ONE_MINUS_SRC_ALPHA          0x0303
#define GL_DST_COLOR                    0x0306
#define GL_FRONT                        0x0404
#define GL_BACK                         0x0405
#define GL_DEPTH_TEST                   0x0B71
#define GL_FOG                          0x0B60
#define GL_FOG_START                    0x0B63
#define GL_FOG_END                      0x0B64
#define GL_FOG_MODE                     0x0B65
#define GL_FOG_COLOR                    0x0B66
#define GL_ALPHA_TEST                   0x0BC0
#define GL_DITHER                       0x0BD0
#define GL_BLEND                        0x0BE2
#define GL_PERSPECTIVE_CORRECTION_HINT  0x0C50
#define GL_FOG_HINT                     0x0C54
#define GL_UNPACK_ALIGNMENT             0x0CF5
#define GL_MAX_TEXTURE_SIZE             0x0D33
#define GL_TEXTURE_2D                   0x0DE1
#define GL_NICEST                       0x1102
#define GL_UNSIGNED_BYTE                0x1401
#define GL_FLOAT                        0x1406
#define GL_MODELVIEW                    0x1700
#define GL_PROJECTION                   0x1701
#define GL_RGB                          0x1907
#define GL_RGBA                         0x1908
#define GL_VERSION                      0x1F02
#define GL_EXTENSIONS                   0x1F03
#define GL_MODULATE                     0x2100
#define GL_DECAL                        0x2101
#define GL_TEXTURE_ENV_MODE             0x2200
#define GL_TEXTURE_ENV                  0x2300
#define GL_LINEAR                       0x2601
#define GL_LINEAR_MIPMAP_LINEAR         0x2703
#define GL_TEXTURE_MAG_FILTER           0x2800
#define GL_TEXTURE_MIN_FILTER           0x2801
#define GL_TEXTURE_WRAP_S               0x2802
#define GL_TEXTURE_WRAP_T               0x2803
#define GL_CLAMP                        0x2900
#define GL_REPEAT                       0x2901
#define GL_UNSIGNED_SHORT_4_4_4_4       0x8033   // OpenGL 1.2 / GL_APPLE_packed_pixel
#define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#define GL_RGBA4                        0x8056
#define GL_RGB5_A1                      0x8057
#define GL_VERTEX_ARRAY                 0x8074
#define GL_COLOR_ARRAY                  0x8076
#define GL_TEXTURE_COORD_ARRAY          0x8078
#define GL_TEXTURE0_ARB                 0x84C0   // GL_ARB_multitexture
#define GL_TEXTURE1_ARB                 0x84C1
#define GL_MAX_TEXTURE_UNITS_ARB        0x84E2

// ---- OPENGL32.dll ---------------------------------------------------------------------------------
#define GL_MIN_API extern "C" __declspec(dllimport)
GL_MIN_API void APIENTRY glAlphaFunc(GLenum func, GLclampf ref);
GL_MIN_API void APIENTRY glArrayElement(GLint i);
GL_MIN_API void APIENTRY glBegin(GLenum mode);
GL_MIN_API void APIENTRY glBindTexture(GLenum target, GLuint texture);
GL_MIN_API void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor);
GL_MIN_API void APIENTRY glClear(GLbitfield mask);
GL_MIN_API void APIENTRY glClearDepth(GLclampd depth);
GL_MIN_API void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GL_MIN_API void APIENTRY glColor4ubv(const GLubyte *v);
GL_MIN_API void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_MIN_API void APIENTRY glCullFace(GLenum mode);
GL_MIN_API void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures);
GL_MIN_API void APIENTRY glDepthFunc(GLenum func);
GL_MIN_API void APIENTRY glDepthMask(GLboolean flag);
GL_MIN_API void APIENTRY glDepthRange(GLclampd zNear, GLclampd zFar);
GL_MIN_API void APIENTRY glDisable(GLenum cap);
GL_MIN_API void APIENTRY glDisableClientState(GLenum array);
GL_MIN_API void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count);
GL_MIN_API void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GL_MIN_API void APIENTRY glEnable(GLenum cap);
GL_MIN_API void APIENTRY glEnableClientState(GLenum array);
GL_MIN_API void APIENTRY glEnd();
GL_MIN_API void APIENTRY glFinish();
GL_MIN_API void APIENTRY glFogf(GLenum pname, GLfloat param);
GL_MIN_API void APIENTRY glFogfv(GLenum pname, const GLfloat *params);
GL_MIN_API void APIENTRY glFogi(GLenum pname, GLint param);
GL_MIN_API void APIENTRY glGetIntegerv(GLenum pname, GLint *params);
GL_MIN_API const GLubyte *APIENTRY glGetString(GLenum name);
GL_MIN_API void APIENTRY glHint(GLenum target, GLenum mode);
GL_MIN_API void APIENTRY glLoadIdentity();
GL_MIN_API void APIENTRY glMatrixMode(GLenum mode);
GL_MIN_API void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
                                 GLdouble zNear, GLdouble zFar);
GL_MIN_API void APIENTRY glPixelStorei(GLenum pname, GLint param);
GL_MIN_API void APIENTRY glPushMatrix();
GL_MIN_API void APIENTRY glReadBuffer(GLenum mode);
GL_MIN_API void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                                      GLenum type, GLvoid *pixels);
GL_MIN_API void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
GL_MIN_API void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_MIN_API void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param);
GL_MIN_API void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width,
                                      GLsizei height, GLint border, GLenum format, GLenum type,
                                      const GLvoid *pixels);
GL_MIN_API void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
GL_MIN_API void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height, GLenum format, GLenum type,
                                         const GLvoid *pixels);
GL_MIN_API void APIENTRY glVertex2i(GLint x, GLint y);
GL_MIN_API void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z);
GL_MIN_API void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_MIN_API void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// ---- GLU32.dll ------------------------------------------------------------------------------------
extern "C" void APIENTRY gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx,
                                   GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy,
                                   GLdouble upz);
extern "C" void APIENTRY gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// GL_ARB_multitexture entry points, fetched with wglGetProcAddress (stdcall: no add esp after the calls).
typedef void (APIENTRY *gl_active_texture_proc)(GLenum texture);   // name ours

#endif
