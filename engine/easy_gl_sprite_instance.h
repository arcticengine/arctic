// The MIT License (MIT)
//
// Copyright (c) 2017 - 2018 Huldra
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

#ifndef ENGINE_EASY_GL_SPRITE_INSTANCE_H_
#define ENGINE_EASY_GL_SPRITE_INSTANCE_H_

#include <memory>
#include <vector>
#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class GLSpriteInstance {
 private:
  Si32 width_;
  Si32 height_;
  Ui32 texture_id_;

 public:
  GLSpriteInstance(Si32 width, Si32 height);
  ~GLSpriteInstance();

  Si32 width() const {
    return width_;
  }

  Si32 height() const {
    return height_;
  }

  Ui32 texture_id() const {
    return texture_id_;
  }

  /// @brief Creates a sprite instance from *.tga file data
  static std::shared_ptr<GLSpriteInstance> LoadTga(const Ui8 *data,
      const Si64 size);

  /// @brief Creates a *.tga file data from a sprite instance
  static void SaveTga(std::shared_ptr<GLSpriteInstance> sprite,
      std::vector<Ui8> *data);
};

/// @}

}  // namespace arctic

extern template class std::shared_ptr<arctic::GLSpriteInstance>;

#endif  // ENGINE_EASY_GL_SPRITE_INSTANCE_H_
