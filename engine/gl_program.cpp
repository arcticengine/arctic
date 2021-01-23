// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
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

#include "engine/gl_program.h"

#include <sstream>
#include "engine/arctic_platform.h"
#include "engine/log.h"
#include "engine/opengl.h"

namespace arctic {

GLuint LoadShader(const char *shaderSrc, GLenum type) {
    // Create the shader object
    GLuint shader;
    ARCTIC_GL_CALL(shader = glCreateShader(type));
    if (shader == 0) {
        Fatal("Can't create shader");
        return 0;
    }
    // Load the shader source
    ARCTIC_GL_CALL(glShaderSource(shader, 1, &shaderSrc, NULL));
    // Compile the shader
    ARCTIC_GL_CALL(glCompileShader(shader));
    // Check the compile status
    GLint compiled;
    ARCTIC_GL_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            //char* infoLog = reinterpret_cast<char*>(
            //    malloc(sizeof(char) * static_cast<size_t>(infoLen)));
            std::string infoLog;
            infoLog.resize(infoLen + 1);
            glGetShaderInfoLog(shader, infoLen, NULL, &infoLog.front());
            Fatal("Error compiling shader: ", infoLog.data());
            //free(infoLog);  //-V779
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


GLProgram::GLProgram()
    : program_id_(0) {
}

GLProgram::~GLProgram() {
    ARCTIC_GL_CALL(glDeleteProgram(program_id_));
}

void GLProgram::Create(const char *vs_src, const char *fs_src) {
    if (program_id_ != 0) {
        ARCTIC_GL_CALL(glDeleteProgram(program_id_));
    }

    // Load the vertex/fragment shaders
    GLuint vertexShader = LoadShader(vs_src, GL_VERTEX_SHADER);
    GLuint fragmentShader = LoadShader(fs_src, GL_FRAGMENT_SHADER);
    // Create the program object
    ARCTIC_GL_CALL(program_id_ = glCreateProgram());
    if (program_id_ == 0) {
        Fatal("Unknown error creating program");
    }
    ARCTIC_GL_CALL(glAttachShader(program_id_, vertexShader));
    ARCTIC_GL_CALL(glAttachShader(program_id_, fragmentShader));
    // Bind vPosition to attribute 0
    ARCTIC_GL_CALL(glBindAttribLocation(program_id_, 0, "vPosition"));
    ARCTIC_GL_CALL(glBindAttribLocation(program_id_, 1, "vTex"));
    // Link the program
    ARCTIC_GL_CALL(glLinkProgram(program_id_));
    // Check the link status
    GLint linked;
    ARCTIC_GL_CALL(glGetProgramiv(program_id_, GL_LINK_STATUS, &linked));
    if (!linked) {
        GLint infoLen = 0;
        ARCTIC_GL_CALL(glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &infoLen));
        if (infoLen > 1) {
            std::string infoLog;
            infoLog.resize(infoLen + 1);
            ARCTIC_GL_CALL(glGetProgramInfoLog(program_id_, infoLen, NULL, &infoLog.front()));
            Fatal("Error linking program: ", infoLog.data());
        }
        ARCTIC_GL_CALL(glDeleteProgram(program_id_));
        Fatal("Unknown error linking program");
    }
}

void GLProgram::Bind() {
    ARCTIC_GL_CALL(glUseProgram(program_id_));
}

void GLProgram::SetUniform(const char *name, int value) {
    GLint loc = glGetUniformLocation(program_id_, name);
    Check(loc >= 0, name, " not found");
    glUniform1i(loc, value);
}

void GLProgram::CheckActiveUniforms() {
    GLint ufs;
    glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &ufs);
    Check(ufs == 1, "no ufs");
}


}  // namespace arctic
