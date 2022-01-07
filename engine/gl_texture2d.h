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

#ifndef ENGINE_GL_TEXTURE2D_H_
#define ENGINE_GL_TEXTURE2D_H_

#include "engine/arctic_types.h"
#include "engine/opengl.h"
#include "engine/easy_sprite.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class GlTexture2D {
 private:
  GlTexture2D(GlTexture2D &other) = delete;
  GlTexture2D(GlTexture2D &&other) = delete;
  GlTexture2D &operator=(GlTexture2D &other) = delete;
  GlTexture2D &operator=(GlTexture2D &&other) = delete;

  Si32 width_;
  Si32 height_;
  GLuint texture_id_;
  DrawFilterMode current_filter_mode_ = kFilterNearest;

  static const GLuint max_textures_slots = 8;
  static GLuint current_texture_slot_;
  static GLuint current_texture_id_[max_textures_slots];

 public:
  GlTexture2D();
  ~GlTexture2D();

  void Create(Si32 w, Si32 h);
  void Bind(Ui32 slot) const;
  void SetData(const void *data, Si32 w, Si32 h);
  void UpdateData(const void *data);
//  void ReadData(void *dst) const;
  void SetFilterMode(DrawFilterMode filter_mode);

  Si32 width() const {
    return width_;
  }

  Si32 height() const {
    return height_;
  }

  GLuint texture_id() const {
    return texture_id_;
  }
};

/// @}

}  // namespace arctic

#endif  // ENGINE_GL_TEXTURE2D_H_
