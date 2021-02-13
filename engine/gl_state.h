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

#ifndef ENGINE_GL_STATE_H_
#define ENGINE_GL_STATE_H_

#include "engine/arctic_types.h"
#include "engine/opengl.h"
#include "engine/easy_sprite.h"
#include "engine/vec4si32.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class GlState {
 private:
  GlState() = delete;
  ~GlState() = delete;
  GlState(GlState &other) = delete;
  GlState(GlState &&other) = delete;
  GlState &operator=(GlState &other) = delete;
  GlState &operator=(GlState &&other) = delete;

  static Vec4Si32 current_viewport_;
  static DrawBlendingMode current_blending_mode_;

 public:
  static void SetViewport(Si32 x, Si32 y, Si32 w, Si32 h);
  static void SetBlending(DrawBlendingMode mode);
};

/// @}

}  // namespace arctic

#endif  // ENGINE_GL_BUFFER_H_
