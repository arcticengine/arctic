// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#ifndef ENGINE_EASY_HW_SPRITE_H_
#define ENGINE_EASY_HW_SPRITE_H_

#include <string>
#include <memory>
#include <vector>

#include "engine/easy_sprite.h"
#include "engine/easy_hw_sprite_instance.h"
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"
#include "engine/rgba.h"

namespace arctic {

/// @addtogroup global_drawing
/// @{

class HwSprite {
 private:
  std::shared_ptr<HwSpriteInstance> sprite_instance_;
  Vec2Si32 ref_pos_;
  Vec2Si32 ref_size_;
  Vec2Si32 pivot_;

 public:
  HwSprite();
  void LoadFromData(const Ui8* data, Ui64 size_bytes, const char *file_name);
  /// @brief Load sprite data from file
  void Load(const char *file_name);
  /// @brief Load sprite data from file
  void Load(const std::string &file_name);
  /// @brief Load sprite data from file
  void Save(const char *file_name);
  /// @brief Load sprite data from file
  void Save(const std::string &file_name);
  std::vector<Ui8> SaveToData(const char *file_name);
  /// @brief Make current sprite an empty sprite of the specified size
  void Create(const Vec2Si32 size);
  /// @brief Make current sprite an empty sprite of the specified size
  void Create(const Si32 width, const Si32 height);
  /// @brief Make current sprite reference to a rectangular part of another sprite using inverted y
  void InvReference(const HwSprite &from, const Si32 from_x, const Si32 from_inv_y,
    const Si32 from_width, const Si32 from_height);
  /// @brief Make current sprite reference to a rectangular part of another sprite
  void Reference(const HwSprite &from, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height);
  /// @brief Fill the sprite with black
  void Clear();
  /// @brief Fill the sprite with the specified color
  void Clear(Rgba color);
  /// @brief Make current sprite a (tansformed) copy of another sprite
  /// @param from Souce sprite to clone from
  /// @param transform A transformation to perform while copying (kCloneUntransformed by default)
  // ******** void Clone(HwSprite from, CloneTransform transform = kCloneUntransformed);
  /// @brief Set the coordinates of the pivot point of the sprite
  void SetPivot(Vec2Si32 pivot);
  /// @brief Get the coordinates of the pivot point of the sprite
  Vec2Si32 Pivot() const;

  void Draw(const Vec2F to, float angle_radians,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));
  void Draw(const float to_x, const float to_y, float angle_radians,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2F to, float angle_radians, float zoom,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));
  void Draw(const float to_x, const float to_y, float angle_radians, float zoom,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));
  void Draw(const float to_x, const float to_y, float angle_radians, float zoom, HwSprite to_sprite,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend, DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Get width of the sprite in pixels
  Si32 Width() const;
  /// @brief Get height of the sprite in pixels
  Si32 Height() const;
  /// @brief Get size of the sprite in pixels
  Vec2Si32 Size() const;
  /// @brief Returns true if the sprite is actually a reference to another sprite
  bool IsRef() const;

  std::shared_ptr<HwSpriteInstance> sprite_instance() const {
    return sprite_instance_;
  }
};

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_HW_SPRITE_H_
