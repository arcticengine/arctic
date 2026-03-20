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

#include <memory>
#include <string>
#include <vector>

#include "engine/easy_sprite.h"
#include "engine/easy_hw_sprite_instance.h"
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"
#include "engine/rgba.h"
#include "engine/gl_program.h"
#include "engine/gl_buffer.h"

namespace arctic {

/// @addtogroup global_drawing
/// @{

struct HwSpriteDrawing;

/// @brief GPU-accelerated sprite for hardware-rendered 2D drawing.
///
/// HwSprite is the hardware-accelerated counterpart of Sprite. While Sprite
/// stores pixel data in CPU memory and performs all drawing on the CPU, HwSprite
/// keeps texture data on the GPU and issues OpenGL draw calls for rendering.
///
/// The Draw() API intentionally mirrors Sprite, so switching from software to
/// hardware rendering in most cases only requires replacing Sprite with HwSprite
/// in your declarations. HwSprite also supports drawing to other HwSprite
/// instances (render-to-texture) and rotation.
///
/// HwSprite can be loaded from image files, created as an empty texture, or
/// converted from an existing software Sprite via LoadFromSoftwareSprite().
class HwSprite {
 private:
  std::shared_ptr<HwSpriteInstance> sprite_instance_;
  Vec2Si32 ref_pos_;
  Vec2Si32 ref_size_;
  Vec2Si32 pivot_;
  mutable std::unique_ptr<GlBuffer> gl_buffer_;
  mutable Vec2Si32 last_buffer_pivot_;
  mutable Vec2Si32 last_buffer_ref_size_;
  mutable float last_angle_;

  static void DrawSprite(
    const HwSprite &to_sprite, const float to_x_pivot, const float to_y_pivot, const float to_width, const float to_height,
    const HwSprite &from_sprite, const float from_x, const float from_y, const float from_width, const float from_height,
    Rgba in_color, DrawBlendingMode blending_mode, DrawFilterMode filter_mode, float angle_radians);
  void UpdateVertexBuffer(float angle) const;

 public:
  HwSprite();
  HwSprite(const HwSprite &other);
  HwSprite(HwSprite &&other) noexcept;

  HwSprite &operator=(const HwSprite &other);
  HwSprite &operator=(HwSprite &&other) noexcept;

  /// @brief Load sprite data from a byte array in memory
  /// @param data Pointer to the byte array containing sprite data
  /// @param size_bytes Size of the byte array in bytes
  /// @param file_name Name of the file (used for error messages and determining format)
  void LoadFromData(const Ui8* data, Ui64 size_bytes, const char *file_name);

  /// @brief Load sprite data from file
  /// @param file_name Name of the file to load
  void Load(const char *file_name);

  /// @brief Load sprite data from file
  /// @param file_name Name of the file to load
  void Load(const std::string &file_name);

  /// @brief Load sprite data from a software Sprite, uploading its pixels to the GPU
  /// @param sw_sprite The software Sprite whose pixel data will be uploaded
  void LoadFromSoftwareSprite(Sprite sw_sprite);

  /// @brief Make current sprite an empty sprite of the specified size
  /// @param size Size of the sprite in pixels
  void Create(const Vec2Si32 size);

  /// @brief Make current sprite an empty sprite of the specified size
  /// @param width Width of the sprite in pixels
  /// @param height Height of the sprite in pixels
  void Create(const Si32 width, const Si32 height);

  /// @brief Make current sprite reference to a rectangular part of another sprite using inverted y
  /// @param from Source sprite
  /// @param from_x X coordinate of the bottom-left corner of the referenced area
  /// @param from_inv_y Inverted Y coordinate of the bottom-left corner of the referenced area
  /// @param from_width Width of the referenced area
  /// @param from_height Height of the referenced area
  void InvReference(const HwSprite &from, const Si32 from_x, const Si32 from_inv_y,
    const Si32 from_width, const Si32 from_height);

  /// @brief Make current sprite reference to a rectangular part of another sprite
  /// @param from Source sprite
  /// @param from_x X coordinate of the bottom-left corner of the referenced area
  /// @param from_y Y coordinate of the bottom-left corner of the referenced area
  /// @param from_width Width of the referenced area
  /// @param from_height Height of the referenced area
  void Reference(const HwSprite &from, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height);

  /// @brief Fill the sprite with black
  void Clear();

  /// @brief Fill the sprite with the specified color
  /// @param color Color to fill the sprite with
  void Clear(Rgba color);

  /// @brief Make current sprite a (transformed) copy of another sprite
  /// @param from Source sprite to clone from
  /// @param transform A transformation to perform while copying (kCloneUntransformed by default)
  void Clone(const HwSprite &from, CloneTransform transform = kCloneUntransformed);

  /// @brief Set the coordinates of the pivot point of the sprite
  /// @param pivot New pivot point coordinates
  void SetPivot(Vec2Si32 pivot);

  /// @brief Get the coordinates of the pivot point of the sprite
  /// @return Pivot point coordinates
  Vec2Si32 Pivot() const;

  /// @brief Draw the sprite to another HwSprite
  /// @param to_sprite Destination sprite
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const HwSprite &to_sprite, const Si32 to_x, const Si32 to_y,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Si32 to_x, const Si32 to_y,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer with scaling
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param to_width Width of the destination area
  /// @param to_height Height of the destination area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Si32 to_x, const Si32 to_y,
      const Si32 to_width, const Si32 to_height,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer with scaling and source rectangle
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param to_width Width of the destination area
  /// @param to_height Height of the destination area
  /// @param from_x X coordinate of the bottom-left corner of the source area
  /// @param from_y Y coordinate of the bottom-left corner of the source area
  /// @param from_width Width of the source area
  /// @param from_height Height of the source area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Si32 to_x, const Si32 to_y,
      const Si32 to_width, const Si32 to_height,
      const Si32 from_x, const Si32 from_y,
      const Si32 from_width, const Si32 from_height,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to another HwSprite with scaling and source rectangle
  /// @param to_sprite Destination sprite
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param to_width Width of the destination area
  /// @param to_height Height of the destination area
  /// @param from_x X coordinate of the bottom-left corner of the source area
  /// @param from_y Y coordinate of the bottom-left corner of the source area
  /// @param from_width Width of the source area
  /// @param from_height Height of the source area
  /// @param blending_mode Blending mode to use
  /// @param filter_mode Filter mode to use
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize
  void Draw(const HwSprite &to_sprite, const Si32 to_x, const Si32 to_y,
      const Si32 to_width, const Si32 to_height,
      const Si32 from_x, const Si32 from_y,
      const Si32 from_width, const Si32 from_height,
      DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color);

  /// @brief Draw the sprite to another HwSprite
  /// @param to_sprite Destination sprite
  /// @param to_pos Position of the bottom-left corner of the destination area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const HwSprite &to_sprite, const Vec2Si32 to_pos,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to another HwSprite with scaling
  /// @param to_sprite Destination sprite
  /// @param to_pos Position of the bottom-left corner of the destination area
  /// @param to_size Size of the destination area
  /// @param blending_mode Blending mode to use
  /// @param filter_mode Filter mode to use
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const HwSprite &to_sprite, const Vec2Si32 to_pos, const Vec2Si32 to_size,
      DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer
  /// @param to_pos Position of the bottom-left corner of the destination area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Vec2Si32 to_pos,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer with scaling
  /// @param to_pos Position of the bottom-left corner of the destination area
  /// @param to_size Size of the destination area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer with scaling and source rectangle
  /// @param to_pos Position of the bottom-left corner of the destination area
  /// @param to_size Size of the destination area
  /// @param from_pos Position of the bottom-left corner of the source area
  /// @param from_size Size of the source area
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
      const Vec2Si32 from_pos, const Vec2Si32 from_size,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to another HwSprite with scaling and source rectangle
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param to_width Width of the destination area
  /// @param to_height Height of the destination area
  /// @param from_x X coordinate of the bottom-left corner of the source area
  /// @param from_y Y coordinate of the bottom-left corner of the source area
  /// @param from_width Width of the source area
  /// @param from_height Height of the source area
  /// @param to_sprite Destination sprite
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Si32 to_x, const Si32 to_y,
      const Si32 to_width, const Si32 to_height,
      const Si32 from_x, const Si32 from_y,
      const Si32 from_width, const Si32 from_height,
      const HwSprite &to_sprite, DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff)) const;

  /// @brief Draw the sprite to the backbuffer at a specific position with rotation
  /// @param to Position of the bottom-left corner of the destination area
  /// @param angle_radians Angle in radians to rotate the sprite (0 by default)
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Vec2F to, float angle_radians = 0.f,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer at a specific position with rotation
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param angle_radians Angle in radians to rotate the sprite (0 by default)
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const float to_x, const float to_y, float angle_radians = 0.f,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer at a specific position with rotation and size
  /// @param to Position of the bottom-left corner of the destination area
  /// @param to_wdith Width of the destination area
  /// @param to_hegiht Height of the destination area
  /// @param angle_radians Angle in radians to rotate the sprite
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const Vec2F to, float to_wdith, float to_hegiht, float angle_radians,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to the backbuffer at a specific position with rotation and size
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param to_wdith Width of the destination area
  /// @param to_hegiht Height of the destination area
  /// @param angle_radians Angle in radians to rotate the sprite
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  void Draw(Rgba in_color, const float to_x, const float to_y, float to_wdith, float to_hegiht, float angle_radians,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest);

  [[deprecated("Use Draw(Rgba in_color, const float to_x, const float to_y, float angle_radians, float zoom, DrawBlendingMode blending_mode, DrawFilterMode filter_mode)")]]
  void Draw(const float to_x, const float to_y, float to_width, float  to_height, float angle_radians,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend,
      DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Draw the sprite to another HwSprite at a specific position with rotation and size
  /// @param to_x X coordinate of the bottom-left corner of the destination area
  /// @param to_y Y coordinate of the bottom-left corner of the destination area
  /// @param to_width Width of the destination area
  /// @param to_height Height of the destination area
  /// @param angle_radians Angle in radians to rotate the sprite
  /// @param to_sprite Destination sprite
  /// @param blending_mode Blending mode to use (kDrawBlendingModeAlphaBlend by default)
  /// @param filter_mode Filter mode to use (kFilterNearest by default)
  /// @param in_color Color to use for drawing, applied in kDrawBlendingModeColorize (0xffffffff by default)
  void Draw(const float to_x, const float to_y, float to_width, float to_height, float angle_radians, const HwSprite &to_sprite,
      DrawBlendingMode blending_mode = kDrawBlendingModeAlphaBlend, DrawFilterMode filter_mode = kFilterNearest,
      Rgba in_color = Rgba(0xffffffff));

  /// @brief Get width of the sprite in pixels
  /// @return Width of the sprite
  Si32 Width() const;

  /// @brief Get height of the sprite in pixels
  /// @return Height of the sprite
  Si32 Height() const;

  /// @brief Get size of the sprite in pixels
  /// @return Size of the sprite
  Vec2Si32 Size() const;

  /// @brief Get the position of the sub-region within the underlying texture
  /// @return Reference position
  Vec2Si32 RefPos() const;

  /// @brief Returns true if the sprite is actually a reference to another sprite
  /// @return True if the sprite is a reference, false otherwise
  bool IsRef() const;

  /// @brief Get the shared pointer to the HwSpriteInstance
  /// @return Shared pointer to HwSpriteInstance
  const std::shared_ptr<HwSpriteInstance> &sprite_instance() const {
    return sprite_instance_;
  }
};

/// @brief A record describing a single batched HwSprite draw command.
///
/// Used internally by the engine to accumulate draw calls for batch submission.
struct HwSpriteDrawing {
    float to_x_pivot;  ///< X coordinate of the pivot point in the destination
    float to_y_pivot;  ///< Y coordinate of the pivot point in the destination
    float to_width;  ///< Width of the destination area
    float to_height;  ///< Height of the destination area
    HwSprite from_sprite;  ///< Source sprite to draw from
    float from_x;  ///< X coordinate of the source area origin
    float from_y;  ///< Y coordinate of the source area origin
    float from_width;  ///< Width of the source area
    float from_height;  ///< Height of the source area
    Rgba in_color;  ///< Color multiplier for drawing
    DrawBlendingMode blending_mode;  ///< Blending mode for the draw call
    DrawFilterMode filter_mode;  ///< Texture filter mode for the draw call
    float angle_radians;  ///< Rotation angle in radians
};

/// @}

}  // namespace arctic

#endif  // ENGINE_EASY_HW_SPRITE_H_
