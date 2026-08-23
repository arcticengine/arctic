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

#include <initializer_list>
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

  mutable std::unordered_map<std::string, Si32> attribute_locations_;

  static GLuint current_program_id_;

 public:
  GlProgram();
  ~GlProgram();

  /// @brief Compiles and links a program, binding the 2d attribute names
  /// @param vs_src Vertex shader source code
  /// @param fs_src Fragment shader source code
  /// @details Binds "vPosition" to slot 0 and "vTex" to slot 1, which is what
  /// the built-in 2d shaders of the engine expect. A shader with other
  /// attribute names, a 3d one for example, gets its locations assigned by the
  /// driver in an unspecified order, so use the overload taking the attribute
  /// names, or ask for the locations with GetAttribLocation.
  void Create(const char *vs_src, const char *fs_src);

  /// @brief Compiles and links a program, binding the given attribute names
  /// @param vs_src Vertex shader source code
  /// @param fs_src Fragment shader source code
  /// @param attribute_names Attribute names to bind, in slot order
  /// @param attribute_count Number of names in attribute_names
  /// @details The name at index i is bound to slot i before linking, so the
  /// vertex data may be bound to the very same slots. Names that the shader
  /// does not declare are silently ignored by OpenGL.
  void Create(const char *vs_src, const char *fs_src,
    const char *const *attribute_names, Si32 attribute_count);

  /// @brief Compiles and links a program, binding the given attribute names
  /// @param vs_src Vertex shader source code
  /// @param fs_src Fragment shader source code
  /// @param attribute_names Attribute names to bind, in slot order
  /// @details Lets one write
  /// Create(vs, fs, {"vPosition", "vNormal", "vTexCoord"}).
  void Create(const char *vs_src, const char *fs_src,
    std::initializer_list<const char*> attribute_names);

  void Bind();
  static void InvalidateCache();

  /// @name Uniform setters
  /// @details There is no setter for an array of uniforms on purpose. Arrays are
  /// a weak spot of some OpenGL ES and WebGL drivers, which report the whole
  /// array as one active uniform, or refuse to give a location for an element,
  /// so CheckActiveUniforms and GetUniformLocation would disagree with the
  /// shader. A shader meant to run on a phone or in a browser is better off with
  /// a uniform per lamp, light0Pos and light1Pos rather than lightPos[2].
  /// @{
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
  void SetUniform(const char *name, const Mat44F &value);
  void SetUniformTransposed(const char *name, const Mat44F &value);
  /// @}
  void CheckActiveUniforms(int required_count);
  int GetUniformLocation(const char *name) const;

  /// @brief Returns the vertex attribute slot the shader uses for a name
  /// @param name Attribute name as written in the vertex shader
  /// @return The slot number, or -1 if the shader has no such attribute
  /// @details Unlike GetUniformLocation this does not stop the program on a
  /// missing name, because skipping an absent attribute is a normal thing to
  /// do while binding vertex data. The answer is cached, so it is cheap to ask
  /// every frame, which matters on the web where every GL call goes to JS.
  Si32 GetAttribLocation(const char *name) const;
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
