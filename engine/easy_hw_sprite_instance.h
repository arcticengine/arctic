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

#ifndef ENGINE_EASY_HW_SPRITE_INSTANCE_H_
#define ENGINE_EASY_HW_SPRITE_INSTANCE_H_

#include <memory>
#include <vector>
#include "engine/arctic_types.h"
#include "engine/gl_texture2d.h"
#include "engine/gl_framebuffer.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

class HwSpriteInstance {
 private:
  GlTexture2D texture_;
  GlFramebuffer framebuffer_;

 public:
  HwSpriteInstance(Si32 width, Si32 height);

  GlTexture2D &texture() {
    return texture_;
  }

  GlFramebuffer &framebuffer() {
    return framebuffer_;
  }

  Si32 width() const {
    return texture_.width();
  }

  Si32 height() const {
      return texture_.height();
  }

  /// @brief Creates a sprite instance from *.tga file data
  static std::shared_ptr<HwSpriteInstance> LoadTga(const Ui8 *data,
      const Si64 size);

  /// @brief Creates a *.tga file data from a sprite instance
  static void SaveTga(std::shared_ptr<HwSpriteInstance> sprite,
      std::vector<Ui8> *data);
};

/// @}

}  // namespace arctic

extern template class std::shared_ptr<arctic::HwSpriteInstance>;

#endif  // ENGINE_EASY_HW_SPRITE_INSTANCE_H_
