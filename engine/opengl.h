// The MIT License (MIT)
//
// Copyright (c) 2016 - 2019 Huldra
// Copyright (c) 2021 Vlad2001_MFS
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef ENGINE_OPENGL_H_
#define ENGINE_OPENGL_H_

#include "engine/arctic_platform_def.h"

#ifdef ARCTIC_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "engine/glext.h"

#define ARCTIC_GL_CHECK_ERROR(opengl_call) do { \
    opengl_call; int call_line = __LINE__; \
    GLenum error_code = glGetError(); \
    if (error_code != GL_NO_ERROR) { \
        *Log() << "OpenGL Error: " << #opengl_call << " -> " << GlErrorToString(error_code) << " (" << error_code << ")" \
               << "\nFile: " << __FILE__ << "\nLine: " << call_line; \
    } \
} while(false)

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;

#endif  // ARCTIC_PLATFORM_WINDOWS

#ifdef ARCTIC_PLATFORM_MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif  // ARCTIC_PLATFORM_MACOSX

#ifdef ARCTIC_PLATFORM_PI_OPENGL_GLX
#include <GL/gl.h>  // NOLINT
#include <GL/glu.h>  // NOLINT
#endif  // ARCTIC_PLATFORM_PI_OPENGL_GLX

#ifdef ARCTIC_PLATFORM_PI_ES_EGL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif  // ARCTIC_PLATFORM_PI_ES_EGL

inline const char *GlErrorToString(GLenum error_code) {
    switch (error_code) {
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        default:                               return "UNKNOWN_ERROR_CODE"; break;
    }
}

#endif  // ENGINE_OPENGL_H_
