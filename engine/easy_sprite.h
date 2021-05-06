// The MIT License (MIT)
//
// Copyright (c) 2017 - 2021 Huldra
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

#ifndef ENGINE_EASY_SPRITE_H_
#define ENGINE_EASY_SPRITE_H_

#include <string>
#include <memory>
#include <vector>

#include "engine/easy_sprite_instance.h"
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"
#include "engine/rgba.h"

namespace arctic {

/// @addtogroup global_drawing
/// @{

enum DrawBlendingMode {
  kCopyRgba [[deprecated("Use kDrawBlendingModeCopyRgba instead of kCopyRgba")]] = 0,  // Deprecated
  kDrawBlendingModeCopyRgba = 0,
  kAlphaBlend [[deprecated("Use kDrawBlendingModeAlphaBlend instead of kAlphaBlend")]] = 1,  // Deprecated
  kDrawBlendingModeAlphaBlend = 1,
  kColorize [[deprecated("User kDrawBlendingModeColorize instead of kColorize")]] = 2,  // Deprecated
  kDrawBlendingModeColorize = 2,
  kDrawBlendingModeAdd = 3,
  kDrawBlendingModeSolidColor = 4,
  kDrawBlendingModePremultipliedAlphaBlend = 5
};

enum DrawFilterMode {
  kFilterNearest,
  kFilterBilinear
};

enum CloneTransform {
  kCloneUntransformed,
  kCloneRotateCw90,
  kCloneRotateCcw90,
  kCloneRotate180,
  kCloneMirrorLr,
  kCloneMirrorUd
};

class Sprite {
 private:
  std::shared_ptr<SpriteInstance> sprite_instance_;
  Vec2Si32 ref_pos_;
  Vec2Si32 ref_size_;
  Vec2Si32 pivot_;

 public:
  Sprite();
  /// @brief Load sprite data from a byte array in memory
  void LoadFromData(const Ui8* data, Ui64 size_bytes, const char *file_name);
  /// @brief Load sprite data from file
  void Load(const char *file_name);
  /// @brief Load sprite data from file
  void Load(const std::string &file_name);
  /// @brief Store sprite data to a file
  void Save(const char *file_name);
  /// @brief Store sprite data to a file
  void Save(const std::string &file_name);
  /// @brief Store sprite data to a byte vector
  /// @param file_name A requeired parameter, may not be nullptr,
  /// the extension of the file_name is used to deremine the desired data format.
  /// At the moment only *.tga is supported.
  /// The file_name is also used in error messages.
  std::vector<Ui8> SaveToData(const char *file_name);
  /// @brief Make current sprite an empty sprite of the specified size
  void Create(const Vec2Si32 size);
  /// @brief Make current sprite an empty sprite of the specified size
  void Create(const Si32 width, const Si32 height);
  /// @brief Make current sprite reference to a rectangular part of another sprite using inverted y
  void InvReference(const Sprite &from, const Si32 from_x, const Si32 from_inv_y,
    const Si32 from_width, const Si32 from_height);
  /// @brief Make current sprite reference to a rectangular part of another sprite
  void Reference(const Sprite &from, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height);
  /// @brief Fill the sprite with black
  void Clear();
  /// @brief Fill the sprite with the specified color
  void Clear(Rgba color);
  /// @brief Make current sprite a (tansformed) copy of another sprite
  /// @param from Souce sprite to clone from
  /// @param transform A transformation to perform while copying (kCloneUntransformed by default)
  void Clone(Sprite from, CloneTransform transform = kCloneUntransformed);
  /// @brief Set the coordinates of the pivot point of the sprite
  void SetPivot(Vec2Si32 pivot);
  /// @brief Get the coordinates of the pivot point of the sprite
  Vec2Si32 Pivot() const;
  void Draw(Sprite to_sprite, const Si32 to_x, const Si32 to_y,
    DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
    DrawFilterMode filter_mode = kFilterNearest,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
    DrawFilterMode filter_mode = kFilterNearest,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(Sprite to_sprite, const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color);
  void Draw(Sprite to_sprite, const Vec2Si32 to_pos,
    DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
    DrawFilterMode filter_mode = kFilterNearest,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(Sprite to_sprite, const Vec2Si32 to_pos, const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to_pos,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    const Vec2Si32 from_pos, const Vec2Si32 from_size,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
            Sprite to_sprite, DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff)) const;
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
  void Draw(Rgba in_color, const float to_x, const float to_y, float angle_radians, float zoom,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest);
  [[deprecated("Use the Draw(Rgba in_color, const float to_x, const float to_y, float angle_radians, float zoom, DrawBlendingMode blending_mode, DrawFilterMode filter_mode)")]]
  void Draw(const float to_x, const float to_y, float angle_radians, float zoom,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const float to_x, const float to_y,
            float angle_radians, float zoom, Sprite to_sprite,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));

  [[deprecated("Use the Draw(const Vec2F to, ...) overload")]]
  void Draw(const Vec2Si32 to, float angle_radians,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff)) {
    Draw(in_color, static_cast<float>(to.x), static_cast<float>(to.y),
        angle_radians, 1.f, blending_mode, filter_mode);
  }
  [[deprecated("Use the Draw(const Vec2F to, ...) overload")]]
  void Draw(const Si32 to_x, const Si32 to_y, float angle_radians,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff)) {
    Draw(in_color, static_cast<float>(to_x), static_cast<float>(to_y),
        angle_radians, 1.f, blending_mode, filter_mode);
  }
  [[deprecated("Use the Draw(const Vec2F to, ...) overload")]]
  void Draw(const Vec2Si32 to, float angle_radians, float zoom,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff)) {
    Draw(in_color, static_cast<float>(to.x), static_cast<float>(to.y),
        angle_radians, zoom, blending_mode, filter_mode);
  }
  [[deprecated("Use the Draw(const float to_x, const float to_y, ...) overload")]]
  void Draw(const Si32 to_x, const Si32 to_y,
            float angle_radians, float zoom,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff)) {
    Draw(in_color, static_cast<float>(to_x), static_cast<float>(to_y),
        angle_radians, zoom, blending_mode, filter_mode);
  }
  [[deprecated("Use the Draw(const float to_x, const float to_y, ...) overload")]]
  void Draw(const Si32 to_x, const Si32 to_y,
            float angle_radians, float zoom, Sprite to_sprite,
            DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff)) {
    Draw(static_cast<float>(to_x), static_cast<float>(to_y),
        angle_radians, zoom, to_sprite, blending_mode, filter_mode, in_color);
  }

  Vec2Si32 RefPos() const;
  /// @brief Get width of the sprite in pixels
  Si32 Width() const;
  /// @brief Get height of the sprite in pixels
  Si32 Height() const;
  /// @brief Get size of the sprite in pixels
  Vec2Si32 Size() const;
  /// @brief Get stride of the sprite raw data in bytes
  Si32 StrideBytes() const;
  /// @brief Get stride of the sprite rgba data in pixels
  Si32 StridePixels() const;
  /// @brief Returns true if the sprite is actually a reference to another sprite
  bool IsRef() const;
  /// @brief Returns a pointer to the raw data bytes of the sprite
  Ui8* RawData();
  /// @brief Returns a pointer to the Rgba data of the sprite
  Rgba* RgbaData();
  /// @brief Returns a pointer to the read-only Rgba data of the sprite
  const Rgba* RgbaData() const;
  const std::shared_ptr<SpriteInstance> &SpriteInstance() const;
  const std::vector<SpanSi32> &Opaque() const;
  /// @brief Update the opaque span parameters of the sprite
  /// @details
  /// Sets up the span parameters so that currently transparent pixels will not be drawn.
  /// Changing pixel transparency may require calling either UpdateOpaqueSpans or ClearOpaqueSpans.
  void UpdateOpaqueSpans();
  /// @brief Clear the opaque span parameters of the sprite so that each pixel of the sprite is drawn
  void ClearOpaqueSpans();
};

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_SPRITE_H_
