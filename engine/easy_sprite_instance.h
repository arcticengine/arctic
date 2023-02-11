// The MIT License (MIT)
//
// Copyright (c) 2017 - 2018 Huldra
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

#ifndef ENGINE_EASY_SPRITE_INSTANCE_H_
#define ENGINE_EASY_SPRITE_INSTANCE_H_

#include <memory>
#include <vector>
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

struct SpanSi32 {
  Si32 begin;
  Si32 end;
};

class SpriteInstance {
 private:
  Si32 width_;
  Si32 height_;
  std::vector<Ui8> data_;
  std::vector<SpanSi32> opaque_;

 public:
  SpriteInstance(Si32 width, Si32 height);

  Si32 width() const {
    return width_;
  }

  Si32 height() const {
    return height_;
  }

  Ui8 *RawData() {
    return data_.data();
  }

  const std::vector<SpanSi32> &Opaque() {
    return opaque_;
  }

  void UpdateOpaqueSpans();
  void ClearOpaqueSpans();
};



/// @brief Creates a sprite instance from *.tga file data
std::shared_ptr<SpriteInstance> LoadTga(const Ui8 *data,
    const Si64 size, Vec2Si32 *out_origin = nullptr);

/// @brief Creates a *.tga file data from a sprite instance
void SaveTga(std::shared_ptr<SpriteInstance> sprite,
    std::vector<Ui8> *data);

/// @}

}  // namespace arctic


#endif  // ENGINE_EASY_SPRITE_INSTANCE_H_
