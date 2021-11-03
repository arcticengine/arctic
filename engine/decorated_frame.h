// The MIT License (MIT)
//
// Copyright (c) 2019 Huldra
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


#ifndef ENGINE_DECORATED_FRAME_H_
#define ENGINE_DECORATED_FRAME_H_

#include "engine/easy_sprite.h"

namespace arctic {


/// @addtogroup global_drawing
/// @{
class DecoratedFrame {
  Sprite left_;
  Sprite right_;
  Sprite top_;
  Sprite bottom_;
  Sprite upper_left_;
  Sprite upper_right_;
  Sprite lower_left_;
  Sprite lower_right_;
  Sprite background_tile_;
  bool is_x_scaleable_ = true;
  bool is_y_scaleable_ = true;

 public:
  DecoratedFrame() {
  }

  void Load(Sprite left, Sprite right,
      Sprite top, Sprite bottom,
      Sprite upper_left, Sprite upper_right,
      Sprite lower_left, Sprite lower_right,
      Sprite background,
      bool is_x_scaleable, bool is_y_scaleable) {
    left_ = left;
    right_ = right;
    top_ = top;
    bottom_ = bottom;
    upper_left_ = upper_left;
    upper_right_ = upper_right;
    lower_left_ = lower_left;
    lower_right_ = lower_right;
    background_tile_ = background;
    is_x_scaleable_ = is_x_scaleable;
    is_y_scaleable_ = is_y_scaleable;
  }

  void Split(Sprite s, Si32 border_size,
      bool is_x_scaleable, bool is_y_scaleable) {
    //    | w |
    // 0  x1  x2 x3
    // a---------b y3
    // |  c---d  | y2 -
    // |  |   |  |    h
    // |  e---f  | y1 -
    // g---------k 0
    Si32 min_size = std::min(s.Width(), s.Height());
    if (min_size < border_size * 2) {
      border_size = min_size / 2;
    }
    Si32 x1 = border_size;
    Si32 x2 = s.Width() - border_size;
    Si32 y1 = border_size;
    Si32 y2 = s.Height() - border_size;
    Si32 w = s.Width() - border_size * 2;
    Si32 h = s.Height() - border_size * 2;
    left_.Reference(s, 0, y1, border_size, h);
    right_.Reference(s, x2, y1, border_size, h);
    top_.Reference(s, x1, y2, w, border_size);
    bottom_.Reference(s, x1, 0, w, border_size);
    upper_left_.Reference(s, 0, y2, border_size, border_size);
    upper_right_.Reference(s, x2, y2, border_size, border_size);
    lower_left_.Reference(s, 0, 0, border_size, border_size);
    lower_right_.Reference(s, x2, 0, border_size, border_size);
    background_tile_.Reference(s, x1, y1, w, h);
    is_x_scaleable_ = is_x_scaleable;
    is_y_scaleable_ = is_y_scaleable;
  }

  Vec2Si32 EstimateSizeForClienArea(Vec2Si32 client_area_size) {
    Vec2Si32 size_bound = client_area_size;
    if (!is_x_scaleable_) {
      size_bound.x = (client_area_size.x + top_.Width() - 1) /
        top_.Width() * top_.Width();
    }
    if (!is_y_scaleable_) {
      size_bound.y = (client_area_size.y + left_.Height() - 1) /
        left_.Height() * left_.Height();
    }
    Vec2Si32 size;
    size.x = size_bound.x + left_.Width() + right_.Width();
    size.y = size_bound.y + top_.Height() + bottom_.Height();
    return size;
  }

  Vec2Si32 ClientAreaForSize(Vec2Si32 size) {
    Vec2Si32 size_bound;
    size_bound.x = size.x - left_.Width() - right_.Width();
    size_bound.y = size.y - top_.Height() - bottom_.Height();
    return size_bound;
  }

  Sprite DrawClientSize(Vec2Si32 client_area_size) {
    Vec2Si32 size = EstimateSizeForClienArea(client_area_size);
    client_area_size = ClientAreaForSize(size);
    return DrawBothSizes(client_area_size, size);
  }

  Sprite DrawExternalSize(Vec2Si32 size) {
    Vec2Si32 client_area_size = ClientAreaForSize(size);
    return DrawBothSizes(client_area_size, size);
  }

  Sprite DrawBothSizes(Vec2Si32 client_size, Vec2Si32 size) {
    Sprite sprite;
    sprite.Create(size.x, size.y);
    sprite.Clear(Rgba(0xff000000));
    // Tile background.
    Vec2Si32 pos(0, 0);
    Vec2Si32 offset(left_.Width(), bottom_.Height());
    if (background_tile_.Size().x && background_tile_.Size().y) {
      while (pos.y < client_size.y) {
        while (pos.x < client_size.x) {
          Vec2Si32 instance_size(
            std::min(background_tile_.Width(), client_size.x - pos.x),
            std::min(background_tile_.Height(), client_size.y - pos.y));
          background_tile_.Draw(pos.x + offset.x, pos.y + offset.y,
            instance_size.x, instance_size.y,
            0, 0, instance_size.x, instance_size.y,
            sprite, kDrawBlendingModeCopyRgba);
          pos.x += background_tile_.Width();
        }
        pos.y += background_tile_.Height();
        pos.x = 0;
      }
    }
    if (top_.Width()) {
      for (Si32 x = 0; x < client_size.x; x += top_.Width()) {
        Si32 width = is_x_scaleable_ ?
          std::min(top_.Width(), client_size.x - x) :
          top_.Width();
        top_.Draw(x + left_.Width(), size.y - top_.Height(),
          width, top_.Height(),
          0, 0, width, top_.Height(), sprite, kDrawBlendingModeCopyRgba);
      }
    }
    if (bottom_.Width()) {
      for (Si32 x = 0; x < client_size.x; x += bottom_.Width()) {
        Si32 width = is_x_scaleable_ ?
          std::min(bottom_.Width(), client_size.x - x) :
          bottom_.Width();
        bottom_.Draw(x + left_.Width(), 0,
          width, bottom_.Height(),
          0, 0, width, bottom_.Height(), sprite, kDrawBlendingModeCopyRgba);
      }
    }
    if (left_.Height()) {
      for (Si32 y = 0; y < client_size.y; y += left_.Height()) {
        left_.Draw(0, y + bottom_.Height(),
          left_.Width(), left_.Height(),
          0, 0, left_.Width(), left_.Height(),
          sprite, kDrawBlendingModeCopyRgba);
      }
    }
    if (right_.Height()) {
      for (Si32 y = 0; y < client_size.y; y += right_.Height()) {
        right_.Draw(size.x - right_.Width(), y + bottom_.Height(),
          right_.Width(), right_.Height(),
          0, 0, right_.Width(), right_.Height(),
          sprite, kDrawBlendingModeCopyRgba);
      }
    }
    lower_left_.Draw(0, 0,
      lower_left_.Width(), lower_left_.Height(),
      0, 0, lower_left_.Width(), lower_left_.Height(),
      sprite, kDrawBlendingModeCopyRgba);
    lower_right_.Draw(size.x - lower_right_.Width(), 0,
      lower_right_.Width(), lower_right_.Height(),
      0, 0, lower_right_.Width(), lower_right_.Height(),
      sprite, kDrawBlendingModeCopyRgba);
    upper_left_.Draw(0, size.y - upper_left_.Height(),
      upper_left_.Width(), upper_left_.Height(),
      0, 0, upper_left_.Width(), upper_left_.Height(),
      sprite, kDrawBlendingModeCopyRgba);
    upper_right_.Draw(size.x - upper_right_.Width(),
      size.y - upper_right_.Height(),
      upper_right_.Width(), upper_right_.Height(),
      0, 0, upper_right_.Width(), upper_right_.Height(), sprite,
      kDrawBlendingModeCopyRgba);

    return sprite;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_DECORATED_FRAME_H_
