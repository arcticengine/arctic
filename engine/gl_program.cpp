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
#include <cstring>

namespace arctic {

GLuint GlProgram::current_program_id_ = 0;

GLuint LoadShader(const char *shaderSrc, GLenum type) {
    // Create the shader object
    GLuint shader;
    ARCTIC_GL_CHECK_ERROR(shader = glCreateShader(type));
    if (shader == 0) {
        Fatal("Can't create shader");
        return 0;
    }
    // Load the shader source
    ARCTIC_GL_CHECK_ERROR(glShaderSource(shader, 1, &shaderSrc, NULL));
    // Compile the shader
    ARCTIC_GL_CHECK_ERROR(glCompileShader(shader));
    // Check the compile status
    GLint compiled;
    ARCTIC_GL_CHECK_ERROR(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));
    if (!compiled) {
        GLint infoLen = 0;
        ARCTIC_GL_CHECK_ERROR(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen));
        if (infoLen > 1) {
            //char* infoLog = reinterpret_cast<char*>(
            //    malloc(sizeof(char) * static_cast<size_t>(infoLen)));
            std::string infoLog;
            infoLog.resize(infoLen + 1);
            ARCTIC_GL_CHECK_ERROR(glGetShaderInfoLog(shader, infoLen, NULL, &infoLog.front()));
            Fatal("Error compiling shader: ", infoLog.data());
            //free(infoLog);  //-V779
        }
        ARCTIC_GL_CHECK_ERROR(glDeleteShader(shader));
        return 0;
    }
    return shader;
}


GlProgram::GlProgram()
    : program_id_(0) {
}

GlProgram::~GlProgram() {
    ARCTIC_GL_CHECK_ERROR(glDeleteProgram(program_id_));
}

void GlProgram::Create(const char *vs_src, const char *fs_src) {
    if (program_id_ != 0) {
        ARCTIC_GL_CHECK_ERROR(glDeleteProgram(program_id_));
    }

    // Load the vertex/fragment shaders
    GLuint vertexShader = LoadShader(vs_src, GL_VERTEX_SHADER);
    GLuint fragmentShader = LoadShader(fs_src, GL_FRAGMENT_SHADER);
    // Create the program object
    ARCTIC_GL_CHECK_ERROR(program_id_ = glCreateProgram());
    if (program_id_ == 0) {
        Fatal("Unknown error creating program");
    }
    ARCTIC_GL_CHECK_ERROR(glAttachShader(program_id_, vertexShader));
    ARCTIC_GL_CHECK_ERROR(glAttachShader(program_id_, fragmentShader));
    // Bind vPosition to attribute 0
    ARCTIC_GL_CHECK_ERROR(glBindAttribLocation(program_id_, 0, "vPosition"));
    ARCTIC_GL_CHECK_ERROR(glBindAttribLocation(program_id_, 1, "vTex"));
    // Link the program
    ARCTIC_GL_CHECK_ERROR(glLinkProgram(program_id_));
    // Check the link status
    GLint linked;
    ARCTIC_GL_CHECK_ERROR(glGetProgramiv(program_id_, GL_LINK_STATUS, &linked));
    if (!linked) {
        GLint infoLen = 0;
        ARCTIC_GL_CHECK_ERROR(glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &infoLen));
        if (infoLen > 1) {
            std::string infoLog;
            infoLog.resize(infoLen + 1);
            ARCTIC_GL_CHECK_ERROR(glGetProgramInfoLog(program_id_, infoLen, NULL, &infoLog.front()));
            Fatal("Error linking program: ", infoLog.data());
        }
        ARCTIC_GL_CHECK_ERROR(glDetachShader(program_id_, vertexShader));
        ARCTIC_GL_CHECK_ERROR(glDetachShader(program_id_, fragmentShader));
        ARCTIC_GL_CHECK_ERROR(glDeleteShader(vertexShader));
        ARCTIC_GL_CHECK_ERROR(glDeleteShader(fragmentShader));
        ARCTIC_GL_CHECK_ERROR(glDeleteProgram(program_id_));
        program_id_ = 0;
        Fatal("Unknown error linking program");
    }
    // Shaders are no longer needed after successful linking.
    ARCTIC_GL_CHECK_ERROR(glDetachShader(program_id_, vertexShader));
    ARCTIC_GL_CHECK_ERROR(glDetachShader(program_id_, fragmentShader));
    ARCTIC_GL_CHECK_ERROR(glDeleteShader(vertexShader));
    ARCTIC_GL_CHECK_ERROR(glDeleteShader(fragmentShader));
}

void GlProgram::Bind() {
    if (current_program_id_ != program_id_) {
        current_program_id_ = program_id_;
        ARCTIC_GL_CHECK_ERROR(glUseProgram(program_id_));
    }
}

void GlProgram::SetUniform(int id, int value) {
    ARCTIC_GL_CHECK_ERROR(glUniform1i(id, value));
}

void GlProgram::SetUniform(int id, const Vec2Si32 &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform2i(id, value.x, value.y));
}

void GlProgram::SetUniform(int id, const Vec3Si32 &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform3i(id, value.x, value.y, value.z));
}

void GlProgram::SetUniform(int id, const Vec4Si32 &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform4i(id, value.x, value.y, value.z, value.w));
}

void GlProgram::SetUniform(int id, float value) {
    ARCTIC_GL_CHECK_ERROR(glUniform1f(id, value));
}

void GlProgram::SetUniform(int id, const Vec2F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform2f(id, value.x, value.y));
}

void GlProgram::SetUniform(int id, const Vec3F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform3f(id, value.x, value.y, value.z));
}

void GlProgram::SetUniform(int id, const Vec4F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform4f(id, value.x, value.y, value.z, value.w));
}

void GlProgram::SetUniform(const char *name, int value) {
    ARCTIC_GL_CHECK_ERROR(glUniform1i(GetUniformLocation(name), value));
}

void GlProgram::SetUniform(const char *name, const Vec2Si32 &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform2i(GetUniformLocation(name), value.x, value.y));
}

void GlProgram::SetUniform(const char *name, const Vec3Si32 &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform3i(GetUniformLocation(name), value.x, value.y, value.z));
}

void GlProgram::SetUniform(const char *name, const Vec4Si32 &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform4i(GetUniformLocation(name), value.x, value.y, value.z, value.w));
}

void GlProgram::SetUniform(const char *name, float value) {
    ARCTIC_GL_CHECK_ERROR(glUniform1f(GetUniformLocation(name), value));
}

void GlProgram::SetUniform(const char *name, const Vec2F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform2f(GetUniformLocation(name), value.x, value.y));
}

void GlProgram::SetUniform(const char *name, const Vec3F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform3f(GetUniformLocation(name), value.x, value.y, value.z));
}

void GlProgram::SetUniform(const char *name, const Vec4F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w));
}

void GlProgram::SetUniform(const char *name, const Mat44F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, value.m));
}

void GlProgram::SetUniformTransposed(const char *name, const Mat44F &value) {
    ARCTIC_GL_CHECK_ERROR(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_TRUE, value.m));
}

void GlProgram::CheckActiveUniforms(int required_count) {
    GLint ufs;
    ARCTIC_GL_CHECK_ERROR(glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &ufs));
    Check(ufs == required_count, "Number of active uniforms does not match the required_count");
}

int GlProgram::GetUniformLocation(const char *name) const {
    GLint loc;
    ARCTIC_GL_CHECK_ERROR(loc = glGetUniformLocation(program_id_, name));
    Check(loc >= 0, name, " not found");
    return loc;
}



UniformsTable::UniformData::UniformData() : type(UniformDataType::Int), value({ 0 }) {
}

UniformsTable::UniformData &UniformsTable::UniformData::operator=(const UniformData &other) {
    type = other.type;
    memcpy((char*)&value, (char*)&other.value, sizeof(value));
    return *this;
}

void UniformsTable::Apply(GlProgram &program) const {
    for (auto &uniform : table_) {
        auto &name = uniform.first;
        auto &data = uniform.second;

        switch (data.type) {
            case UniformDataType::Int:
                program.SetUniform(name.data(), data.value.i);
                break;
            case UniformDataType::Int2:
                program.SetUniform(name.data(), data.value.i2);
                break;
            case UniformDataType::Int3:
                program.SetUniform(name.data(), data.value.i3);
                break;
            case UniformDataType::Int4:
                program.SetUniform(name.data(), data.value.i4);
                break;
            case UniformDataType::Float:
                program.SetUniform(name.data(), data.value.f);
                break;
            case UniformDataType::Float2:
                program.SetUniform(name.data(), data.value.f2);
                break;
            case UniformDataType::Float3:
                program.SetUniform(name.data(), data.value.f3);
                break;
            case UniformDataType::Float4:
                program.SetUniform(name.data(), data.value.f4);
                break;
        }
    }
}

void UniformsTable::Clear() {
    table_.clear();
}

void UniformsTable::SetUniform(const std::string &name, int value) {
    UniformData data;
    data.type = UniformDataType::Int;
    data.value.i = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, const Vec2Si32 &value) {
    UniformData data;
    data.type = UniformDataType::Int2;
    data.value.i2 = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, const Vec3Si32 &value) {
    UniformData data;
    data.type = UniformDataType::Int3;
    data.value.i3 = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, const Vec4Si32 &value) {
    UniformData data;
    data.type = UniformDataType::Int4;
    data.value.i4 = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, float value) {
    UniformData data;
    data.type = UniformDataType::Float;
    data.value.f = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, const Vec2F &value) {
    UniformData data;
    data.type = UniformDataType::Float2;
    data.value.f2 = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, const Vec3F &value) {
    UniformData data;
    data.type = UniformDataType::Float3;
    data.value.f3 = value;
    table_[name] = data;
}

void UniformsTable::SetUniform(const std::string &name, const Vec4F &value) {
    UniformData data;
    data.type = UniformDataType::Float4;
    data.value.f4 = value;
    table_[name] = data;
}

size_t UniformsTable::Size() const {
    return table_.size();
}

}  // namespace arctic
