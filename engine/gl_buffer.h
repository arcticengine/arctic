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

#ifndef ENGINE_GL_BUFFER_H_
#define ENGINE_GL_BUFFER_H_

#include "engine/arctic_types.h"
#include "engine/opengl.h"
#include "engine/easy_sprite.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class GlBuffer {
 private:
  GlBuffer(GlBuffer &other) = delete;
  GlBuffer(GlBuffer &&other) = delete;
  GlBuffer &operator=(GlBuffer &other) = delete;
  GlBuffer &operator=(GlBuffer &&other) = delete;

  GLuint buffer_id_;
  size_t size_;

  static GLuint current_buffer_id_;

 public:
  GlBuffer();
  ~GlBuffer();

  void Create(const void *data, size_t size);
  void Bind() const;
  void SetData(const void *data, size_t size);
  void UpdateData(const void *data);

  bool IsValid() const {
    return buffer_id_ != 0;
  }

  GLuint buffer_id() const {
    return buffer_id_;
  }

  static void BindDefault();
};

/// @}

}  // namespace arctic

#endif  // ENGINE_GL_BUFFER_H_
