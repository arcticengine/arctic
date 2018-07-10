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

#include "engine/easy_sprite.h"

#include <string.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "engine/easy.h"
#include "engine/rgba.h"

namespace arctic {
namespace easy {

// Just like MasterBoy wrote in HUGi 17, but without subpixel
// see http://www.hugi.scene.org/online/coding/hugi%2017%20-%20cotriang.htm
// or http://www.hugi.scene.org/online/hugi17/
template<DrawBlendingMode kBlendingMode>
inline void DrawTrianglePart(Rgba *dst, Si32 stride,
    float *x1, float *x2, Vec2F *tex_a, Vec2F *tex_b,
    float dxdy1, float dxdy2,
    Vec2F dtdy1, Vec2F dtdy2,
    Si32 width, Si32 height,
    Si32 y1, Si32 y2, Sprite texture, Rgba in_color) {
  Si32 y = y1;
  if (y1 < 0) {
    Si32 yc = std::min(0, y2);
    float d = static_cast<float>(yc - y);
    y = yc;
    *x1 += dxdy1 * d;
    *x2 += dxdy2 * d;
    *tex_a += dtdy1 * d;
    *tex_b += dtdy2 * d;
  }
  Si32 ye = std::min(height, y2);
  dst += y * stride;
  Si32 tex_stride = texture.StridePixels();
  Rgba *tex_data = texture.RgbaData();
  for (; y < ye; y++) {
    Si32 x1i = static_cast<Si32>(*x1);
    Si32 x2i = static_cast<Si32>(*x2);

    Si32 x12i = x2i - x1i;
    Si32 x1c = std::max(0, x1i);
    Si32 x2c = std::min(width, x2i);

    if (x2c <= x1c) {
      if (x2c == x1c && x2c < width) {
        Vec2F tex_ab = *tex_b - *tex_a;
        Vec2F tex_1c = (x12i == 0 ? *tex_a :
          *tex_a + tex_ab * static_cast<float>(x1c - x1i) /
          static_cast<float>(x12i));
        Si32 offset = static_cast<Si32>(tex_1c.x) +
          static_cast<Si32>(tex_1c.y) * tex_stride;
        Rgba *p = dst + x1c;
        if (kBlendingMode == kCopyRgba) {
          p->rgba = tex_data[offset].rgba;
        } else if (kBlendingMode == kAlphaBlend) {
          Rgba color = tex_data[offset];
          if (color.a == 255) {
            p->rgba = color.rgba;
          } else if (color.a) {
            Ui32 m = 255 - color.a;
            Ui32 rb = (p->rgba & 0x00ff00fful) * m;
            Ui32 g = ((p->rgba & 0x0000ff00ul) >> 8) * m;
            Ui32 m2 = color.a;
            Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
            Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8) * m2;
            p->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
              ((g + g2) & 0x0000ff00ul);
          }
        } else if (kBlendingMode == kColorize) {
          Rgba color = tex_data[offset];
          Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
          if (ca == 255) {
            Ui32 r2 = Ui32(color.r) * (Ui32(in_color.r) + 1) >> 8;
            Ui32 g2 = Ui32(color.g) * (Ui32(in_color.g) + 1) >> 8;
            Ui32 b2 = Ui32(color.b) * (Ui32(in_color.b) + 1) >> 8;
            p->rgba = Rgba(r2, g2, b2).rgba;
          } else if (ca) {
            Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
            Ui32 m = 255 - ca;
            Ui32 rb = (p->rgba & 0x00ff00fful) * m;
            Ui32 g = ((p->rgba & 0x0000ff00ul) >> 8) * m;
            
            Ui32 m2 = ca;
            Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
            Ui32 g2 = Ui32(color.g) * m2 * (Ui32(in_color.g) + 1) >> 8;
            Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
            Ui32 rb2 = ((r2 >> 8) & 0xff00) + (b2 & 0xff000000);
            
            p->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
              ((g + g2) & 0x0000ff00ul);
          }
        } else {
          p->rgba = tex_data[offset].rgba;
        }
      }
    } else {
      Vec2F tex_ab = *tex_b - *tex_a;
      Vec2F tex_1c = (x1c == x1i ? *tex_a :
        *tex_a + tex_ab * static_cast<float>(x1c - x1i) /
        static_cast<float>(x12i));
      Vec2F tex_2c = (x2c == x2i ? *tex_b :
        *tex_a + tex_ab * static_cast<float>(x2c - x1i) /
        static_cast<float>(x12i));
      Vec2F tex_1c2c = tex_2c - tex_1c;

      Vec2Si32 tex_16(tex_1c * 65536.f);
      Vec2Si32 tex_12_16(tex_1c2c * 65536.f);
      Vec2Si32 tex_12_16_step = tex_12_16 / (x2c - x1c);

      Rgba *p = dst + x1c;
      for (Si32 x = x1c; x < x2c; ++x) {
        Si32 offset = (tex_16.x >> 16) +
          (tex_16.y >> 16) * tex_stride;
        if (kBlendingMode == kCopyRgba) {
          p->rgba = tex_data[offset].rgba;
        } else if (kBlendingMode == kAlphaBlend) {
          Rgba color = tex_data[offset];
          if (color.a == 255) {
            p->rgba = color.rgba;
          } else if (color.a) {
            Ui32 m = 255 - color.a;
            Ui32 rb = (p->rgba & 0x00ff00fful) * m;
            Ui32 g = ((p->rgba & 0x0000ff00ul) >> 8) * m;
            Ui32 m2 = color.a;
            Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
            Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8) * m2;
            p->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
              ((g + g2) & 0x0000ff00ul);
          }
        } else if (kBlendingMode == kColorize) {
          Rgba color = tex_data[offset];
          Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
          if (ca == 255) {
            Ui32 r2 = Ui32(color.r) * (Ui32(in_color.r) + 1) >> 8;
            Ui32 g2 = Ui32(color.g) * (Ui32(in_color.g) + 1) >> 8;
            Ui32 b2 = Ui32(color.b) * (Ui32(in_color.b) + 1) >> 8;
            p->rgba = Rgba(r2, g2, b2).rgba;
          } else if (ca) {
            Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
            Ui32 m = 255 - ca;
            Ui32 rb = (p->rgba & 0x00ff00fful) * m;
            Ui32 g = ((p->rgba & 0x0000ff00ul) >> 8) * m;
            
            Ui32 m2 = ca;
            Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
            Ui32 g2 = Ui32(color.g) * m2 * (Ui32(in_color.g) + 1) >> 8;
            Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
            Ui32 rb2 = ((r2 >> 8) & 0xff00) + (b2 & 0xff000000);
            
            p->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
            ((g + g2) & 0x0000ff00ul);
          }
        } else {
          p->rgba = tex_data[offset].rgba;
        }
        p++;
        tex_16 += tex_12_16_step;
      }
    }
    *x1 += dxdy1;
    *x2 += dxdy2;
    *tex_a += dtdy1;
    *tex_b += dtdy2;
    dst += stride;
  }
}

template<DrawBlendingMode kBlendingMode>
void DrawTriangle(Vec2Si32 a, Vec2Si32 b, Vec2Si32 c,
  Vec2F tex_a, Vec2F tex_b, Vec2F tex_c,
  Sprite texture, Sprite to_sprite, Rgba in_color) {
  if (a.y > b.y) {
    std::swap(a, b);
    std::swap(tex_a, tex_b);
  }
  if (a.y > c.y) {
    std::swap(a, c);
    std::swap(tex_a, tex_c);
  }
  if (b.y > c.y) {
    std::swap(b, c);
    std::swap(tex_b, tex_c);
  }
  if (a.y == c.y) {
    return;
  }
  Si32 stride = to_sprite.StridePixels();
  Rgba *dst = to_sprite.RgbaData();
  Si32 width = to_sprite.Width();
  Si32 height = to_sprite.Height();

  float dxdy_ac = static_cast<float>(c.x - a.x) /
    static_cast<float>(c.y - a.y);
  float dxdy_bc = static_cast<float>(c.x - b.x) /
    static_cast<float>(c.y - b.y);
  float dxdy_ab = static_cast<float>(b.x - a.x) /
    static_cast<float>(b.y - a.y);

  Vec2F dcdy_ac = (tex_c - tex_a) / static_cast<float>(c.y - a.y);
  Vec2F dcdy_bc = (tex_c - tex_b) / static_cast<float>(c.y - b.y);
  Vec2F dcdy_ab = (tex_b - tex_a) / static_cast<float>(b.y - a.y);

  float x1;
  float x2;
  float dxdy1;
  float dxdy2;
  Vec2F tex1;
  Vec2F tex2;
  Vec2F dtdy1;
  Vec2F dtdy2;

  bool is_b_at_the_right_side = dxdy_ac < dxdy_ab;
  if (is_b_at_the_right_side) {
    dxdy1 = dxdy_ac;
    dtdy1 = dcdy_ac;
    if (a.y == b.y) {
      dxdy2 = dxdy_bc;
      x1 = static_cast<float>(a.x);
      x2 = static_cast<float>(b.x);
      dtdy2 = dcdy_bc;
      tex1 = tex_a;
      tex2 = tex_b;
      DrawTrianglePart<kBlendingMode>(dst, stride, &x1, &x2, &tex1, &tex2,
        dxdy1, dxdy2,
        dtdy1, dtdy2, width, height, a.y, c.y, texture, in_color);
      return;
    }
    if (a.y < b.y) {
      dxdy2 = dxdy_ab;
      x1 = static_cast<float>(a.x);
      x2 = static_cast<float>(a.x);
      dtdy2 = dcdy_ab;
      tex1 = tex_a;
      tex2 = tex_a;
      DrawTrianglePart<kBlendingMode>(dst, stride, &x1, &x2, &tex1, &tex2,
        dxdy1, dxdy2,
        dtdy1, dtdy2, width, height, a.y, b.y, texture, in_color);
    }
    if (b.y < c.y) {
      dxdy2 = dxdy_bc;
      x2 = static_cast<float>(b.x);
      dtdy2 = dcdy_bc;
      tex2 = tex_b;
      DrawTrianglePart<kBlendingMode>(dst, stride, &x1, &x2, &tex1, &tex2,
        dxdy1, dxdy2,
        dtdy1, dtdy2, width, height, b.y, c.y, texture, in_color);
    }
  } else {
    // b is at the left side
    dxdy2 = dxdy_ac;
    dtdy2 = dcdy_ac;
    if (a.y == b.y) {
      dxdy1 = dxdy_bc;
      x1 = static_cast<float>(b.x);
      x2 = static_cast<float>(a.x);
      dtdy1 = dcdy_bc;
      tex1 = tex_b;
      tex2 = tex_a;
      DrawTrianglePart<kBlendingMode>(dst, stride, &x1, &x2, &tex1, &tex2,
        dxdy1, dxdy2,
        dtdy1, dtdy2, width, height, a.y, c.y, texture, in_color);
      return;
    }
    if (a.y < b.y) {
      dxdy1 = dxdy_ab;
      x1 = static_cast<float>(a.x);
      x2 = static_cast<float>(a.x);
      dtdy1 = dcdy_ab;
      tex1 = tex_a;
      tex2 = tex_a;
      DrawTrianglePart<kBlendingMode>(dst, stride, &x1, &x2, &tex1, &tex2,
        dxdy1, dxdy2,
        dtdy1, dtdy2, width, height, a.y, b.y, texture, in_color);
    }
    if (b.y < c.y) {
      dxdy1 = dxdy_bc;
      x1 = static_cast<float>(b.x);
      dtdy1 = dcdy_bc;
      tex1 = tex_b;
      DrawTrianglePart<kBlendingMode>(dst, stride, &x1, &x2, &tex1, &tex2,
        dxdy1, dxdy2,
        dtdy1, dtdy2, width, height, b.y, c.y, texture, in_color);
    }
  }
}

template<DrawBlendingMode kBlendingMode>
void DrawSprite(Sprite to_sprite,
    const Si32 to_x_pivot, const Si32 to_y_pivot,
    const Si32 to_width, const Si32 to_height,
    Sprite from_sprite, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    Rgba in_color) {
  if (!from_width || !from_height || !to_width || !to_height) {
    return;
  }
  const Si32 from_stride_pixels = from_sprite.StridePixels();
  const Si32 to_stride_pixels = to_sprite.Width();

  if (to_width == from_width && to_height == from_height
      && !from_sprite.IsRef() && !from_sprite.Opaque().empty()) {
    const std::vector<SpanSi32> &opaque = from_sprite.Opaque();

    const Si32 to_x = to_x_pivot - from_sprite.Pivot().x;
    const Si32 to_y = to_y_pivot - from_sprite.Pivot().y;

    Rgba *to = to_sprite.RgbaData()
      + to_y * to_stride_pixels
      + to_x;
    const Rgba *from = from_sprite.RgbaData()
      + from_y * from_stride_pixels
      + from_x;

    const Si32 to_y_db = (to_y >= 0 ? 0 : -to_y);
    const Si32 to_y_d_max = to_sprite.Height() - to_y;
    const Si32 to_y_de = (to_height < to_y_d_max ? to_height : to_y_d_max);

    const Si32 k_to_x_db = (to_x >= 0 ? 0 : -to_x);
    const Si32 to_x_d_max = to_sprite.Width() - to_x;
    const Si32 k_to_x_de = (to_width < to_x_d_max ? to_width : to_x_d_max);
    const Si32 from_x_ab = k_to_x_db + from_x;
    const Si32 from_x_ae = k_to_x_de + from_x;

    for (Si32 to_y_disp = to_y_db; to_y_disp < to_y_de; ++to_y_disp) {
      const Si32 from_y_disp = to_y_disp;

      Si32 from_x_acc = 0;
      const SpanSi32 &span = opaque[from_y + to_y_disp];

      Si32 to_x_db = k_to_x_db;
      if (span.begin > from_x_ab) {
        to_x_db += span.begin - from_x_ab;
      }
      Si32 to_x_de = k_to_x_de;
      if (span.end < from_x_ae) {
        Si32 offset = span.end - from_x_ae;
        to_x_de += offset;
      }

      const Rgba *from_line = from + from_y_disp * from_stride_pixels;
      Rgba *to_line = to + to_y_disp * to_stride_pixels;

      for (Si32 to_x_disp = to_x_db; to_x_disp < to_x_de; ++to_x_disp) {
        Rgba *to_rgba = to_line + to_x_disp;
        const Si32 from_x_disp = to_x_db + from_x_acc;
        from_x_acc++;
        const Rgba *from_rgba = from_line + from_x_disp;
        Rgba color = *from_rgba;
        if (kBlendingMode == kCopyRgba) {
          to_rgba->rgba = from_rgba->rgba;
        } else if (kBlendingMode == kAlphaBlend) {
          if (color.a == 255) {
            to_rgba->rgba = from_rgba->rgba;
          } else if (color.a) {
            Ui32 m = 255 - color.a;
            Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
            Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8) * m;
            Ui32 m2 = color.a;
            Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
            Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8) * m2;
            to_rgba->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
            ((g + g2) & 0x0000ff00ul);
          }
        } else if (kBlendingMode == kColorize) {
          Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
          if (ca == 255) {
            Ui32 r2 = Ui32(color.r) * (Ui32(in_color.r) + 1) >> 8;
            Ui32 g2 = Ui32(color.g) * (Ui32(in_color.g) + 1) >> 8;
            Ui32 b2 = Ui32(color.b) * (Ui32(in_color.b) + 1) >> 8;
            to_rgba->rgba = Rgba(r2, g2, b2).rgba;
          } else if (ca) {
            Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
            Ui32 m = 255 - ca;
            Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
            Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8) * m;
            
            Ui32 m2 = ca;
            Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
            Ui32 g2 = Ui32(color.g) * m2 * (Ui32(in_color.g) + 1) >> 8;
            Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
            Ui32 rb2 = ((r2 >> 8) & 0xff00) + (b2 & 0xff000000);
            
            to_rgba->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
              ((g + g2) & 0x0000ff00ul);
          }
        } else {  // Unknown blending mode!
          to_rgba->rgba = from_rgba->rgba;
        }
      }
    }
    return;
  }

  const Si32 to_x = to_x_pivot -
    from_sprite.Pivot().x * to_width / from_width;
  const Si32 to_y = to_y_pivot -
    from_sprite.Pivot().y * to_height / from_height;

  Rgba *to = to_sprite.RgbaData()
    + to_y * to_stride_pixels
    + to_x;
  const Rgba *from = from_sprite.RgbaData()
    + from_y * from_stride_pixels
    + from_x;

  const Si32 to_y_db = (to_y >= 0 ? 0 : -to_y);
  const Si32 to_y_d_max = to_sprite.Height() - to_y;
  const Si32 to_y_de = (to_height < to_y_d_max ? to_height : to_y_d_max);

  const Si32 to_x_db = (to_x >= 0 ? 0 : -to_x);
  const Si32 to_x_d_max = to_sprite.Width() - to_x;
  const Si32 to_x_de = (to_width < to_x_d_max ? to_width : to_x_d_max);

  for (Si32 to_y_disp = to_y_db; to_y_disp < to_y_de; ++to_y_disp) {
    const Si32 from_y_disp = (from_height * to_y_disp) / to_height;

    const Si32 from_x_b = (from_width * to_x_db) / to_width;
    const Si32 from_x_step_16 = 65536 * from_width / to_width;
    Si32 from_x_acc_16 = 0;

    const Rgba *from_line = from + from_y_disp * from_stride_pixels;
    Rgba *to_line = to + to_y_disp * to_stride_pixels;

    for (Si32 to_x_disp = to_x_db; to_x_disp < to_x_de; ++to_x_disp) {
      Rgba *to_rgba = to_line + to_x_disp;
      const Si32 from_x_disp = from_x_b + (from_x_acc_16 / 65536);
      from_x_acc_16 += from_x_step_16;
      const Rgba *from_rgba = from_line + from_x_disp;
      Rgba color = *from_rgba;
      if (kBlendingMode == kCopyRgba) {
        to_rgba->rgba = from_rgba->rgba;
      } else if (kBlendingMode == kAlphaBlend) {
        if (color.a == 255) {
          to_rgba->rgba = from_rgba->rgba;
        } else if (color.a) {
          Ui32 m = 255 - color.a;
          Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
          Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8) * m;
          Ui32 m2 = color.a;
          Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
          Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8) * m2;
          to_rgba->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
            ((g + g2) & 0x0000ff00ul);
        }
      } else if (kBlendingMode == kColorize) {
        Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
        if (ca == 255) {
          Ui32 r2 = Ui32(color.r) * (Ui32(in_color.r) + 1) >> 8;
          Ui32 g2 = (Ui32(color.g) * (Ui32(in_color.g) + 1)) & 0xff00;
          Ui32 b2 = ((Ui32(color.b) * (Ui32(in_color.b) + 1)) << 8) & 0xff0000;
          to_rgba->rgba = r2 | g2 | b2;
        } else if (ca) {
          Ui32 ca = Ui32(color.a) * (Ui32(in_color.a) + 1) >> 8;
          Ui32 m = 255 - ca;
          Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
          Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8) * m;
          
          Ui32 m2 = ca;
          Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
          Ui32 g2 = Ui32(color.g) * m2 * (Ui32(in_color.g) + 1) >> 8;
          Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
          Ui32 rb2 = ((r2 >> 8) & 0xff00) + (b2 & 0xff000000);
          
          to_rgba->rgba = (((rb + rb2) >> 8) & 0x00ff00fful) |
          ((g + g2) & 0x0000ff00ul);
        }
      } else {  // Unknown blending mode!
        to_rgba->rgba = from_rgba->rgba;
      }
    }
  }
}

template void DrawSprite<kCopyRgba>(
  Sprite to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  Sprite from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kAlphaBlend>(
  Sprite to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  Sprite from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kColorize>(
  Sprite to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  Sprite from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);

Sprite::Sprite() {
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(0, 0);
  pivot_ = Vec2Si32(0, 0);
}

void Sprite::Load(const char *file_name) {
  Check(!!file_name, "Error in Sprite::Load, file_name is nullptr.");
  const char *last_dot = strchr(file_name, '.');
  Check(!!last_dot, "Error in Sprite::Load, file_name has no extension.");
  if (strcmp(last_dot, ".tga") == 0) {
    std::vector<Ui8> data = ReadFile(file_name, true);
    if (data.size() == 0) {
      Log("File \"", file_name, "\" could not be loaded. Using empty sprite.");
      return;
    }
    sprite_instance_ = LoadTga(data.data(), data.size());
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(sprite_instance_->width(),
      sprite_instance_->height());
    pivot_ = Vec2Si32(0, 0);
  } else {
    Fatal("Error in Sprite::Load, unknown file extension.");
  }
}

void Sprite::Load(const std::string &file_name) {
  Load(file_name.c_str());
}

void Sprite::Save(const char *file_name) {
  Check(!!file_name, "Error in Sprite::Save, file_name is nullptr.");
  const char *last_dot = strchr(file_name, '.');
  Check(!!last_dot, "Error in Sprite::Save, file_name has no extension.");
  if (strcmp(last_dot, ".tga") == 0) {
    std::vector<Ui8> data;
    SaveTga(sprite_instance_, &data);
    WriteFile(file_name, data.data(), data.size());
  } else {
    Fatal("Error in Sprite::Save, unknown file extension.");
  }
}

void Sprite::Save(const std::string &file_name) {
  Save(file_name.c_str());
}

void Sprite::Create(const Si32 width, const Si32 height) {
  sprite_instance_.reset(new SpriteInstance(width, height));
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(width, height);
  pivot_ = Vec2Si32(0, 0);
  Clear();
}

void Sprite::Reference(Sprite from, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height) {
  ref_pos_ = Vec2Si32(
    from.ref_pos_.x + std::min(std::max(from_x, 0), from.ref_size_.x - 1),
    from.ref_pos_.y + std::min(std::max(from_y, 0), from.ref_size_.y - 1));
  const Vec2Si32 max_size = from.ref_pos_ + from.ref_size_ - ref_pos_;
  ref_size_ = Vec2Si32(
    std::min(from_width, max_size.x),
    std::min(from_height, max_size.y));
  pivot_ = Vec2Si32(0, 0);
  sprite_instance_ = from.sprite_instance_;
}

void Sprite::Clear() {
  if (!sprite_instance_.get()) {
    return;
  }
  const size_t size = static_cast<size_t>(ref_size_.x) * sizeof(Rgba);
  Ui8 *data = sprite_instance_->RawData();
  const Si32 stride = StrideBytes();
  for (Si32 y = 0; y < ref_size_.y; ++y) {
    memset(data, 0, size);
    data += stride;
  }
}

void Sprite::Clear(Rgba color) {
  if (!sprite_instance_.get()) {
    return;
  }
  const Si32 stride = StridePixels();
  Rgba *begin = reinterpret_cast<Rgba*>(sprite_instance_->RawData());
  Rgba *end = begin + ref_size_.x;
  for (Si32 y = 0; y < ref_size_.y; ++y) {
    Rgba *p = begin;
    while (p != end) {
      p->rgba = color.rgba;
      p++;
    }
    begin += stride;
    end += stride;
  }
}

void Sprite::Clone(Sprite from) {
  Create(from.Width(), from.Height());
  from.Draw(0, 0, from.Width(), from.Height(),
    0, 0, from.Width(), from.Height(), *this, kCopyRgba);
  SetPivot(from.Pivot());
}

void Sprite::SetPivot(Vec2Si32 pivot) {
  pivot_ = pivot;
}

Vec2Si32 Sprite::Pivot() const {
  return pivot_;
}

void Sprite::Draw(Sprite to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  DrawBlendingMode blending_mode, Rgba color) {
  if (!sprite_instance_.get()) {
    return;
  }
  switch (blending_mode) {
  case kAlphaBlend:
    DrawSprite<kAlphaBlend>(to_sprite, to_x_pivot, to_y_pivot, Width(), Height(),
      *this, 0, 0, Width(), Height(),
      color);
    break;
  case kCopyRgba:
    DrawSprite<kCopyRgba>(to_sprite, to_x_pivot, to_y_pivot, Width(), Height(),
      *this, 0, 0, Width(), Height(),
      color);
    break;
  case kColorize:
    DrawSprite<kColorize>(to_sprite, to_x_pivot, to_y_pivot, Width(), Height(),
      *this, 0, 0, Width(), Height(),
      color);
    break;
  }
}


void Sprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
    DrawBlendingMode blending_mode, Rgba color) {
  Draw(GetEngine()->GetBackbuffer(), to_x_pivot, to_y_pivot, blending_mode, color);
}

void Sprite::Draw(const Vec2Si32 to, float angle_radians,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to.x, to.y, angle_radians, 1.f, GetEngine()->GetBackbuffer(),
      blending_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y, float angle_radians,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_x, to_y, angle_radians, 1.f, GetEngine()->GetBackbuffer(),
      blending_mode, in_color);
}

void Sprite::Draw(const Vec2Si32 to, float angle_radians, float zoom,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to.x, to.y, angle_radians, zoom, GetEngine()->GetBackbuffer(),
      blending_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    float angle_radians, float zoom,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_x, to_y, angle_radians, zoom, GetEngine()->GetBackbuffer(),
      blending_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    float angle_radians, float zoom, Sprite to_sprite,
    DrawBlendingMode blending_mode, Rgba in_color) {
  if (!sprite_instance_) {
    return;
  }
  Vec2F pivot = Vec2F(Vec2Si32(to_x, to_y));
  float sin_a = sinf(angle_radians) * zoom;
  float cos_a = cosf(angle_radians) * zoom;
  Vec2F left = Vec2F(-cos_a, -sin_a) * static_cast<float>(pivot_.x);
  Vec2F right = Vec2F(cos_a, sin_a) * static_cast<float>(Width() - pivot_.x);
  Vec2F up = Vec2F(-sin_a, cos_a) * static_cast<float>(Height() - pivot_.y);
  Vec2F down = Vec2F(sin_a, -cos_a) * static_cast<float>(pivot_.y);

  // d c
  // a b
  Vec2Si32 a(pivot + left + down + 0.5f);
  Vec2Si32 b(pivot + right + down + 0.5f);
  Vec2Si32 c(pivot + right + up + 0.5f);
  Vec2Si32 d(pivot + left + up + 0.5f);

  Vec2F ta(0.001f,
    0.001f);
  Vec2F tb(static_cast<float>(ref_size_.x) - 0.001f,
    0.001f);
  Vec2F tc(static_cast<float>(ref_size_.x) - 0.001f,
    static_cast<float>(ref_size_.y) - 0.001f);
  Vec2F td(0.001f,
    static_cast<float>(ref_size_.y) - 0.001f);

  switch (blending_mode) {
    case kCopyRgba:
      DrawTriangle<kCopyRgba>(a, b, c, ta, tb, tc, *this, to_sprite, in_color);
      DrawTriangle<kCopyRgba>(c, d, a, tc, td, ta, *this, to_sprite, in_color);
      break;
    case kAlphaBlend:
      DrawTriangle<kAlphaBlend>(a, b, c, ta, tb, tc, *this, to_sprite,
        in_color);
      DrawTriangle<kAlphaBlend>(c, d, a, tc, td, ta, *this, to_sprite,
        in_color);
      break;
    case kColorize:
      DrawTriangle<kColorize>(a, b, c, ta, tb, tc, *this, to_sprite, in_color);
      DrawTriangle<kColorize>(c, d, a, tc, td, ta, *this, to_sprite, in_color);
      break;
  }
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
    0, 0, ref_size_.x, ref_size_.y);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
    from_x, from_y, from_width, from_height,
    GetEngine()->GetBackbuffer());
}

void Sprite::Draw(const Vec2Si32 to_pos, DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, blending_mode, in_color);
}

void Sprite::Draw(Sprite to_sprite, const Vec2Si32 to_pos, DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_sprite, to_pos.x, to_pos.y, blending_mode, in_color);
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
    0, 0, ref_size_.x, ref_size_.y);
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    const Vec2Si32 from_pos, const Vec2Si32 from_size,
    DrawBlendingMode blending_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
    from_pos.x, from_pos.y, from_size.x, from_size.y);
}


void Sprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    Sprite to_sprite, DrawBlendingMode blending_mode, Rgba in_color) {
  switch (blending_mode) {
  default:
  case kCopyRgba:
    DrawSprite<kCopyRgba>(to_sprite, to_x_pivot, to_y_pivot, to_width, to_height,
      *this, from_x, from_y, from_width, from_height,
      in_color);
    break;
  case kAlphaBlend:
    DrawSprite<kAlphaBlend>(to_sprite, to_x_pivot, to_y_pivot, to_width, to_height,
      *this, from_x, from_y, from_width, from_height,
      in_color);
    break;
  case kColorize:
    DrawSprite<kColorize>(to_sprite, to_x_pivot, to_y_pivot, to_width, to_height,
      *this, from_x, from_y, from_width, from_height,
      in_color);
    break;
  }
  return;
}

Si32 Sprite::Width() const {
  return ref_size_.x;
}

Si32 Sprite::Height() const {
  return ref_size_.y;
}

Vec2Si32 Sprite::Size() const {
  return ref_size_;
}

Si32 Sprite::StrideBytes() const {
  return sprite_instance_->width() * sizeof(Rgba);
}

Si32 Sprite::StridePixels() const {
  return sprite_instance_->width();
}

bool Sprite::IsRef() const {
  return (ref_pos_.x
      || ref_pos_.y
      || ref_size_.x != sprite_instance_->width()
      || ref_size_.y != sprite_instance_->height());
}

Ui8* Sprite::RawData() {
  return sprite_instance_->RawData();
}

Rgba* Sprite::RgbaData() {
  return (static_cast<Rgba*>(static_cast<void*>(
    sprite_instance_->RawData())) +
    ref_pos_.y * StridePixels() +
    ref_pos_.x);
}

const std::vector<SpanSi32> &Sprite::Opaque() const {
  return sprite_instance_->Opaque();
}

void Sprite::UpdateOpaqueSpans() {
  sprite_instance_->UpdateOpaqueSpans();
}

void Sprite::ClearOpaqueSpans() {
  sprite_instance_->ClearOpaqueSpans();
}

}  // namespace easy
}  // namespace arctic
