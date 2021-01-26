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

#include "engine/arctic_types.h"
#include "engine/opengl.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class GLProgram {
private:
  GLProgram(GLProgram &other) = delete;
  GLProgram(GLProgram &&other) = delete;
  GLProgram &operator=(GLProgram &other) = delete;
  GLProgram &operator=(GLProgram &&other) = delete;

  GLuint program_id_;

 public:
  GLProgram();
  ~GLProgram();

  void Create(const char *vs_src, const char *fs_src);
  void Bind();
  void SetUniform(const char *name, int value);
  void CheckActiveUniforms();
};

/// @}

}  // namespace arctic

#endif  // ENGINE_GL_PROGRAM_H_
