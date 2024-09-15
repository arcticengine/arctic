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

/// @brief Struct representing a span of integers
struct SpanSi32 {
  Si32 begin;
  Si32 end;
};

/// @brief Class representing a sprite instance
class SpriteInstance {
 private:
  /// @brief Width of the sprite instance
  Si32 width_;
  /// @brief Height of the sprite instance
  Si32 height_;
  /// @brief Raw data of the sprite instance
  std::vector<Ui8> data_;
  /// @brief Opaque spans of the sprite instance
  std::vector<SpanSi32> opaque_;

 public:
  /// @brief Constructor for SpriteInstance
  /// @param width Width of the sprite instance
  /// @param height Height of the sprite instance
  SpriteInstance(Si32 width, Si32 height);

  /// @brief Get the width of the sprite instance
  /// @return Width of the sprite instance
  Si32 width() const {
    return width_;
  }

  /// @brief Get the height of the sprite instance
  /// @return Height of the sprite instance
  Si32 height() const {
    return height_;
  }

  /// @brief Get the raw data of the sprite instance
  /// @return Pointer to the raw data of the sprite instance
  Ui8 *RawData() {
    return data_.data();
  }

  /// @brief Get the opaque spans of the sprite instance
  /// @return Reference to the opaque spans of the sprite instance
  const std::vector<SpanSi32> &Opaque() {
    return opaque_;
  }

  /// @brief Update the opaque spans of the sprite instance
  void UpdateOpaqueSpans();

  /// @brief Clear the opaque spans of the sprite instance
  void ClearOpaqueSpans();
};



/// @brief Creates a sprite instance from *.tga file data
/// @param data Pointer to the *.tga file data
/// @param size Size of the *.tga file data
/// @param out_origin Pointer to the origin of the sprite instance
/// @return Pointer to the sprite instance
std::shared_ptr<SpriteInstance> LoadTga(const Ui8 *data,
    const Si64 size, Vec2Si32 *out_origin = nullptr);

/// @brief Creates a *.tga file data from a sprite instance
/// @param sprite Pointer to the sprite instance
/// @param data Pointer to the vector to store the *.tga file data
void SaveTga(std::shared_ptr<SpriteInstance> sprite,
    std::vector<Ui8> *data);

/// @}

}  // namespace arctic


#endif  // ENGINE_EASY_SPRITE_INSTANCE_H_
