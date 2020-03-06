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
namespace easy {

enum DrawBlendingMode {
  kCopyRgba = 0, // Deprecated
  kDrawBlendingModeCopyRgba = 0,
  kAlphaBlend = 1, // Deprecated
  kDrawBlendingModeAlphaBlend = 1,
  kColorize = 2, // Deprecated
  kDrawBlendingModeColorize = 2
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
  void LoadFromData(const Ui8* data, Ui64 size_bytes, const char *file_name);
  void Load(const char *file_name);
  void Load(const std::string &file_name);
  void Save(const char *file_name);
  void Save(const std::string &file_name);
  std::vector<Ui8> SaveToData(const char *file_name);
  void Create(const Vec2Si32 size);
  void Create(const Si32 width, const Si32 height);
  void Reference(const Sprite &from, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height);
  void Clear();
  void Clear(Rgba color);
  void Clone(Sprite from, CloneTransform transform = kCloneUntransformed);
  void SetPivot(Vec2Si32 pivot);
  Vec2Si32 Pivot() const;
  void Draw(Sprite to_sprite, const Si32 to_x, const Si32 to_y,
    DrawBlendingMode blending_mode = kAlphaBlend,
    DrawFilterMode filter_mode = kFilterNearest,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    DrawBlendingMode blending_mode = kAlphaBlend,
    DrawFilterMode filter_mode = kFilterNearest,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(Sprite to_sprite, const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color);
  void Draw(Sprite to_sprite, const Vec2Si32 to_pos,
    DrawBlendingMode blending_mode = kAlphaBlend,
    DrawFilterMode filter_mode = kFilterNearest,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(Sprite to_sprite, const Vec2Si32 to_pos, const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
    Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to_pos,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    const Vec2Si32 from_pos, const Vec2Si32 from_size,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
            Sprite to_sprite, DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to, float angle_radians,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y, float angle_radians,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Vec2Si32 to, float angle_radians, float zoom,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    float angle_radians, float zoom,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));
  void Draw(const Si32 to_x, const Si32 to_y,
    float angle_radians, float zoom, Sprite to_sprite,
            DrawBlendingMode blending_mode = kAlphaBlend,
            DrawFilterMode filter_mode = kFilterNearest,
            Rgba in_color = Rgba(0xffffffff));

  Si32 Width() const;
  Si32 Height() const;
  Vec2Si32 Size() const;
  Si32 StrideBytes() const;
  Si32 StridePixels() const;
  bool IsRef() const;
  Ui8* RawData();
  Rgba* RgbaData();
  const Rgba* RgbaData() const;
  const std::vector<SpanSi32> &Opaque() const;
  void UpdateOpaqueSpans();
  void ClearOpaqueSpans();
};

}  // namespace easy
}  // namespace arctic

#endif  // ENGINE_EASY_SPRITE_H_
