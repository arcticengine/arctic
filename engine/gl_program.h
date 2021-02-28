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

#ifndef ENGINE_GL_PROGRAM_H_
#define ENGINE_GL_PROGRAM_H_

#include <string>
#include <unordered_map>

#include "engine/arctic_types.h"
#include "engine/arctic_math.h"
#include "engine/opengl.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class GlProgram {
private:
  GlProgram(GlProgram &other) = delete;
  GlProgram(GlProgram &&other) = delete;
  GlProgram &operator=(GlProgram &other) = delete;
  GlProgram &operator=(GlProgram &&other) = delete;

  GLuint program_id_;

  static GLuint current_program_id_;

 public:
  GlProgram();
  ~GlProgram();

  void Create(const char *vs_src, const char *fs_src);
  void Bind();
  void SetUniform(int id, int value);
  void SetUniform(int id, const Vec2Si32 &value);
  void SetUniform(int id, const Vec3Si32 &value);
  void SetUniform(int id, const Vec4Si32 &value);
  void SetUniform(int id, float value);
  void SetUniform(int id, const Vec2F &value);
  void SetUniform(int id, const Vec3F &value);
  void SetUniform(int id, const Vec4F &value);
  void SetUniform(const char *name, int value);
  void SetUniform(const char *name, const Vec2Si32 &value);
  void SetUniform(const char *name, const Vec3Si32 &value);
  void SetUniform(const char *name, const Vec4Si32 &value);
  void SetUniform(const char *name, float value);
  void SetUniform(const char *name, const Vec2F &value);
  void SetUniform(const char *name, const Vec3F &value);
  void SetUniform(const char *name, const Vec4F &value);
  void CheckActiveUniforms(int required_count);
  int GetUniformLocation(const char *name) const;
};

class UniformsTable {
    enum class UniformDataType {
        Int,
        Int2,
        Int3,
        Int4,
        Float,
        Float2,
        Float3,
        Float4,
    };

    struct UniformData {
        UniformData();

        UniformData &operator=(const UniformData &other);

        UniformDataType type;
        union {
            int i;
            Vec2Si32 i2;
            Vec3Si32 i3;
            Vec4Si32 i4;
            float f;
            Vec2F f2;
            Vec3F f3;
            Vec4F f4;
        } value;
    };

    std::unordered_map<std::string, UniformData> table_;

public:
    void Apply(GlProgram &program) const;
    void Clear();
    void SetUniform(const std::string &name, int value);
    void SetUniform(const std::string &name, const Vec2Si32 &value);
    void SetUniform(const std::string &name, const Vec3Si32 &value);
    void SetUniform(const std::string &name, const Vec4Si32 &value);
    void SetUniform(const std::string &name, float value);
    void SetUniform(const std::string &name, const Vec2F &value);
    void SetUniform(const std::string &name, const Vec3F &value);
    void SetUniform(const std::string &name, const Vec4F &value);
    size_t Size() const;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_GL_PROGRAM_H_
