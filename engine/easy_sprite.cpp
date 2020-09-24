// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#include <cstring>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "engine/easy.h"
#include "engine/rgba.h"

namespace arctic {

struct Edge {
  float x;
  Vec2F tex;
};

template<DrawBlendingMode kBlendingMode, DrawFilterMode kFilterMode>
void DrawTriangle(Sprite to_sprite,
    Vec2F a, Vec2F b, Vec2F c,
    Vec2F tex_a, Vec2F tex_b, Vec2F tex_c,
    Sprite texture, Rgba in_color) {
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

  Si32 stride = to_sprite.StridePixels();
  Rgba *dst = to_sprite.RgbaData();
  Si32 width = to_sprite.Width();
  Si32 height = to_sprite.Height();
  float height_f = static_cast<float>(height);
  float width_f = static_cast<float>(width);
  if (c.y < 0 || a.y >= height_f) {
    return;
  }

  Edge *edge = static_cast<Edge*>(alloca(sizeof(Edge) * height * 2));
  Si32 first_y = static_cast<Si32>(a.y);
  Si32 last_y = static_cast<Si32>(c.y);

  if (first_y == last_y) {
    Si32 y = first_y;
    Edge &edge_l = edge[y * 2];
    Edge &edge_r = edge[y * 2 + 1];
    if (b.x < a.x) {
      edge_l.x = b.x;
      edge_l.tex = tex_b;
      edge_r.x = a.x;
      edge_r.tex = tex_a;
    } else {
      edge_l.x = a.x;
      edge_l.tex = tex_a;
      edge_r.x = b.x;
      edge_r.tex = tex_b;
    }
    if (edge_l.x > c.x) {
      edge_l.x = c.x;
      edge_l.tex = tex_c;
    } else if (edge_r.x < c.x) {
      edge_r.x = c.x;
      edge_r.tex = tex_c;
    }
  } else {
    Edge *edge_ac;
    Edge *edge_abc;
    // Assign edges

    bool is_b_at_the_right_side;
    {
      Vec2F ac = c - a;
      float ac_x_at_b_y = a.x + ac.x * (b.y - a.y) / ac.y;
      is_b_at_the_right_side = ac_x_at_b_y < b.x;
      edge_ac = is_b_at_the_right_side ? edge : edge + 1;
      edge_abc = is_b_at_the_right_side ? edge + 1 : edge;
    }

    // Edge ac
    {
      Vec2F ac = c - a;
      Vec2F tex_ac = tex_c - tex_a;
      float y1 = std::max(0.f, a.y);
      float y2 = std::min(height_f - 1.f, c.y);
      Si32 y1_i = static_cast<Si32>(y1);
      Si32 y2_i = static_cast<Si32>(y2);
      if (y1_i > y2_i) {
        // Out of destination sprite
        return;
      }
      first_y = y1_i;
      last_y = y2_i;
      float x1 = a.x + ac.x * (y1 - a.y) / ac.y;
      float x2 = a.x + ac.x * (y2 - a.y) / ac.y;
      Vec2F tex1 = tex_a + tex_ac * (y1 - a.y) / ac.y;
      Vec2F tex2 = tex_a + tex_ac * (y2 - a.y) / ac.y;
      float dy = static_cast<float>(y2_i - y1_i);
      if (dy > 0) {
        //         xCx
        //      xxxxxxB  rb, x2 > x1, x1 then move
        //   xxxxx
        // Axx

        //     xCx
        //    xxxxx  rb, x2 < x1, x1 then move slow
        //   Bxxxxxxx
        //         xxxA

        //         xCx
        //      xxxxxxB  Lb, x1 > x1, x1 then move slow
        //   xxxxx
        // Axx

        //    xC
        // Bxxxxxx  Lb, x2 < x1, x1+ them move
        //     xxxxx
        //         xxA

        //   CxxxxxxxxxB  rb, x2 < x1, x1+ them move
        //     xxxxxxx
        //       xAx
        float dxdy = (x2 - x1) / dy;
        Vec2F dtdy = (tex2 - tex1) / dy;
        {
          Edge &e = edge_ac[y1_i * 2];
          e.x = x1;
          e.tex = tex1;
        }
        float frac_y =  static_cast<float>(y1_i + 1) - y1;
        x1 += dxdy*frac_y;
        tex1 += dtdy*frac_y;
        for (Si32 y = y1_i + 1; y <= y2_i; ++y) {
          Edge &e = edge_ac[y * 2];
          e.x = x1;
          e.tex = tex1;
          x1 += dxdy;
          tex1 += dtdy;
        }
      } else if (dy == 0) {
        if (is_b_at_the_right_side ? x1 > x2 : x2 < x1) {
          Edge &e = edge_ac[y1_i * 2];
          e.x = x1;
          e.tex = tex1;
        } else {
          Edge &e = edge_ac[y1_i * 2];
          e.x = x2;
          e.tex = tex2;
        }

      }
    }
    // Edge ab
    if (b.y >= 0.f) {
      Vec2F ab = b - a;
      Vec2F tex_ab = tex_b - tex_a;
      float y1 = std::max(0.f, a.y);
      float y2 = std::min(height_f - 1.f, b.y);
      Si32 y1_i = static_cast<Si32>(y1);
      Si32 y2_i = static_cast<Si32>(y2);
      if (y1_i < y2_i) {
        float x1 = a.x + ab.x * (y1 - a.y) / ab.y;
        float x2 = a.x + ab.x * (y2 - a.y) / ab.y;
        Vec2F tex1 = tex_a + tex_ab * (y1 - a.y) / ab.y;
        Vec2F tex2 = tex_a + tex_ab * (y2 - a.y) / ab.y;
        float dy = static_cast<float>(y2_i - y1_i);
        float dxdy = (x2 - x1) / dy;
        Vec2F dtdy = (tex2 - tex1) / dy;

        //      xxxxxxB  rb, x2 > x1, x1+ then move
        //   xxxxx
        // Axx

        // xxxxxxB  rb, x2 < x1, x1 then move slow
        //     xxxxx
        //         xxA

        //      Bxxxxxx  Lb, x1 > x1, x1 then move slow
        //   xxxxx
        // Axx

        // Bxxxxxx  Lb, x2 < x1, x1+ them move
        //     xxxxx
        //         xxA

        if (is_b_at_the_right_side ? x2 < x1 : x2 >= x1) {
          // x1 then move slow
          {
            Edge &e = edge_abc[y1_i * 2];
            e.x = x1;
            e.tex = tex1;
          }
          float frac_y =  static_cast<float>(y1_i + 1) - y1;
          x1 += dxdy*frac_y;
          tex1 += dtdy*frac_y;
          for (Si32 y = y1_i + 1; y <= y2_i; ++y) {
            Edge &e = edge_abc[y * 2];
            e.x = x1;
            e.tex = tex1;
            x1 += dxdy;
            tex1 += dtdy;
          }
        } else {
          float frac_y =  static_cast<float>(y1_i + 1) - y1;
          if (frac_y < 1.f) {
            x1 += dxdy * frac_y;
            tex1 += dtdy * frac_y;
          }
          for (Si32 y = y1_i; y < y2_i; ++y) {
            Edge &e = edge_abc[y * 2];
            e.x = x1;
            e.tex = tex1;
            x1 += dxdy;
            tex1 += dtdy;
          }
          Edge &e = edge_abc[y2_i * 2];
          e.x = x2;
          e.tex = tex2;
        }
      } else if (y1_i == y2_i) {
        if (ab.y > 0.f) {
          if (is_b_at_the_right_side) {
            float x2 = a.x + ab.x * (y2 - a.y) / ab.y;
            Vec2F tex2 = tex_a + tex_ab * (y2 - a.y) / ab.y;
            Edge &e = edge_abc[y1_i * 2];
            e.x = x2;
            e.tex = tex2;
          } else {
            float x1 = a.x + ab.x * (y1 - a.y) / ab.y;
            Vec2F tex1 = tex_a + tex_ab * (y1 - a.y) / ab.y;
            Edge &e = edge_abc[y1_i * 2];
            e.x = x1;
            e.tex = tex1;
          }
        } else {
          if (is_b_at_the_right_side) {
            Edge &e = edge_abc[y1_i * 2];
            e.x = b.x;
            e.tex = tex_b;
          } else {
            Edge &e = edge_abc[y1_i * 2];
            e.x = a.x;
            e.tex = tex_a;
          }
        }
      }
    }
    // Edge bc
    if (b.y < height_f) {
      Vec2F bc = c - b;
      Vec2F tex_bc = tex_c - tex_b;
      float y1 = std::max(0.f, b.y);
      float y2 = std::min(height_f - 1.f, c.y);
      Si32 y1_i = static_cast<Si32>(y1);
      Si32 y2_i = static_cast<Si32>(y2);
      if (y1_i < y2_i) {
        float x1 = b.x + bc.x * (y1 - b.y) / bc.y;
        float x2 = b.x + bc.x * (y2 - b.y) / bc.y;
        Vec2F tex1 = tex_b + tex_bc * (y1 - b.y) / bc.y;
        Vec2F tex2 = tex_b + tex_bc * (y2 - b.y) / bc.y;
        float dy = static_cast<float>(y2_i - y1_i);
        float dxdy = (x2 - x1) / dy;
        Vec2F dtdy = (tex2 - tex1) / dy;
        // Cxx
        //   xxxxx
        //      xxxxxxB  rb, x2 < x1, x1 then move slow

        //         xxC
        //     xxxxx
        // xxxxxxB  rb, x2 > x1, x1 then move slow

        // Cxx
        //   xxxxx
        //      Bxxxxxx  Lb, x2 < x1, x1 then move slow

        //         xxC
        //     xxxxx
        // Bxxxxxx  Lb, x2 > x1, x1 then move slow
        {//is_b_at_the_right_side ? x2 < x1 : x2 > x1) {
          {
            Edge &e = edge_abc[y1_i * 2];
            e.x = x1;
            e.tex = tex1;
          }
          float frac_y = static_cast<float>(y1_i + 1) - y1;
          x1 += dxdy * frac_y;
          tex1 += dtdy * frac_y;
          for (Si32 y = y1_i + 1; y <= y2_i; ++y) {
            Edge &e = edge_abc[y * 2];
            e.x = x1;
            e.tex = tex1;
            x1 += dxdy;
            tex1 += dtdy;
          }
        }
      } else if (y1_i == y2_i) {
        if (bc.y > 0.f) {
          if (is_b_at_the_right_side) {
            float x1 = b.x + bc.x * (y1 - b.y) / bc.y;
            Vec2F tex1 = tex_b + tex_bc * (y1 - b.y) / bc.y;
            Edge &e = edge_abc[y1_i * 2];
            e.x = x1;
            e.tex = tex1;
          } else {
            float x2 = b.x + bc.x * (y2 - b.y) / bc.y;
            Vec2F tex2 = tex_b + tex_bc * (y2 - b.y) / bc.y;
            Edge &e = edge_abc[y1_i * 2];
            e.x = x2;
            e.tex = tex2;
          }
        } else {
          Edge &e = edge_abc[y1_i * 2];
          e.x = b.x;
          e.tex = tex_b;
        }
      }
    }
  }

  // Fill
  Si32 tex_stride = texture.StridePixels();
  const Rgba * const tex_data = texture.RgbaData();
  for (Si32 y = first_y; y <= last_y; ++y) {
    const Edge &edge_l = edge[y * 2];
    const Edge &edge_r = edge[y * 2 + 1];
    if (edge_r.x < 0.f || edge_l.x >= width) {
      continue;
    }
    float x1 = std::max(0.f, edge_l.x);
    float x2 = std::min(width_f - 1.f, edge_r.x);
    Si32 x1_i = static_cast<Si32>(x1);
    Si32 x2_i = static_cast<Si32>(x2);

    Vec2F tex1;
    Vec2F dtdx;
    if (x1_i < x2_i) {
      float lr = edge_r.x - edge_l.x;
      Vec2F tex_lr = edge_r.tex - edge_l.tex;
      tex1 = edge_l.tex + tex_lr * (x1 - edge_l.x) / lr;
      Vec2F tex2 = edge_l.tex + tex_lr * (x2 - edge_l.x) / lr;
      float dx = static_cast<float>(x2_i - x1_i);
      dtdx = (tex2 - tex1) / dx;
    } else if (x1_i == x2_i) {
      tex1 = edge_l.tex;
      dtdx = Vec2F(0.f, 0.f);
    } else {
      continue;
    }
    Rgba * const p_line = dst + y * stride;
    for (Si32 x = x1_i; x <= x2_i; ++x) {
      // output to p_line[x]
      // texture at tex1
      Rgba * const to_rgba = p_line + x;
      Rgba color;
      if (kFilterMode == kFilterNearest) {
        color = *(tex_data + static_cast<Si32>(tex1.x + 0.5f) +
                  static_cast<Si32>(tex1.y + 0.5f) * tex_stride);
      } else if (kFilterMode == kFilterBilinear) {
        Rgba color00 = *(tex_data + static_cast<Si32>(tex1.x) +
          static_cast<Si32>(tex1.y) * tex_stride);
        Rgba color01 = *(tex_data + static_cast<Si32>(tex1.x+1) +
          static_cast<Si32>(tex1.y) * tex_stride);
        Rgba color10 = *(tex_data + static_cast<Si32>(tex1.x) +
          static_cast<Si32>(tex1.y+1) * tex_stride);
        Rgba color11 = *(tex_data + static_cast<Si32>(tex1.x+1) +
          static_cast<Si32>(tex1.y+1) * tex_stride);

        Ui32 from_x_8 = static_cast<Ui32>((tex1.x - floorf(tex1.x))*255.f);
        Ui32 from_y_8 = static_cast<Ui32>((tex1.y - floorf(tex1.y))*255.f);
        color = Rgba(
           (Ui8)(((Ui32(color00.r) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.r) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.r) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.r) * ((from_x_8) * (from_y_8)))) >> 16u),
           (Ui8)(((Ui32(color00.g) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.g) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.g) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.g) * ((from_x_8) * (from_y_8)))) >> 16u),
           (Ui8)(((Ui32(color00.b) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.b) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.b) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.b) * ((from_x_8) * (from_y_8)))) >> 16u),
           (Ui8)(((Ui32(color00.a) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.a) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.a) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.a) * ((from_x_8) * (from_y_8)))) >> 16u));
      }


      if (kBlendingMode == kCopyRgba) {
        to_rgba->rgba = color.rgba;
      } else if (kBlendingMode == kAlphaBlend) {
        if (color.a == 255) {
          to_rgba->rgba = color.rgba;;
        } else if (color.a) {
          Ui32 m = 255 - color.a;
          Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
          Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8u) * m;
          Ui32 m2 = color.a;
          Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
          Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8u) * m2;
          to_rgba->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) |
          ((g + g2) & 0x0000ff00ul);
        }
      } else if (kBlendingMode == kColorize) {
        Ui32 ca = (Ui32(color.a) * (Ui32(in_color.a) + 1u)) >> 8u;
        if (ca == 255) {
          Ui32 r2 = (Ui32(color.r) * (Ui32(in_color.r) + 1)) >> 8u;
          Ui32 g2 = (Ui32(color.g) * (Ui32(in_color.g) + 1)) >> 8u;
          Ui32 b2 = (Ui32(color.b) * (Ui32(in_color.b) + 1)) >> 8u;
          to_rgba->rgba = Rgba((Ui8)r2, (Ui8)g2, (Ui8)b2).rgba;
        } else if (ca) {
          Ui32 m = 255 - ca;
          Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
          Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8u) * m;

          Ui32 m2 = ca;
          Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
          Ui32 g2 = (Ui32(color.g) * m2 * (Ui32(in_color.g) + 1)) >> 8u;
          Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
          Ui32 rb2 = ((r2 >> 8u) & 0xff00u) + (b2 & 0xff000000);

          to_rgba->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) |
            ((g + g2) & 0x0000ff00ul);
        }
      } else {  // Unknown blending mode!
        to_rgba->rgba = color.rgba;
      }

      tex1 += dtdx;
    }
  }

}

template<DrawBlendingMode kBlendingMode, DrawFilterMode kFilterMode>
void DrawSprite(Sprite *to_sprite,
    const Si32 to_x_pivot, const Si32 to_y_pivot,
    const Si32 to_width, const Si32 to_height,
    const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    Rgba in_color) {
  if (!from_width || !from_height || !to_width || !to_height) {
    return;
  }
  const Si32 from_stride_pixels = from_sprite.StridePixels();
  const Si32 to_stride_pixels = to_sprite->StridePixels();

  if (to_width == from_width && to_height == from_height
      && !from_sprite.IsRef() && !from_sprite.Opaque().empty()) {
    const std::vector<SpanSi32> &opaque = from_sprite.Opaque();

    const Si32 to_x = to_x_pivot - from_sprite.Pivot().x;
    const Si32 to_y = to_y_pivot - from_sprite.Pivot().y;

    Rgba *to = to_sprite->RgbaData()
      + to_y * to_stride_pixels
      + to_x;
    const Rgba *from = from_sprite.RgbaData()
      + from_y * from_stride_pixels
      + from_x;

    const Si32 to_y_db = (to_y >= 0 ? 0 : -to_y);
    const Si32 to_y_d_max = to_sprite->Height() - to_y;
    const Si32 to_y_de = (to_height < to_y_d_max ? to_height : to_y_d_max);

    const Si32 k_to_x_db = (to_x >= 0 ? 0 : -to_x);
    const Si32 to_x_d_max = to_sprite->Width() - to_x;
    const Si32 k_to_x_de = (to_width < to_x_d_max ? to_width : to_x_d_max);
    const Si32 from_x_ab = k_to_x_db + from_x;
    const Si32 from_x_ae = k_to_x_de + from_x;

    for (Si32 to_y_disp = to_y_db; to_y_disp < to_y_de; ++to_y_disp) {
      const Si32 from_y_disp = to_y_disp;

      Si32 from_x_acc = 0;
      const SpanSi32 &span = opaque[static_cast<size_t>(from_y + to_y_disp)];

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
            to_rgba->rgba = color.rgba;
          } else if (color.a) {
            Ui32 m = 255 - color.a;
            Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
            Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8u) * m;
            Ui32 m2 = color.a;
            Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
            Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8u) * m2;
            to_rgba->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) |
            ((g + g2) & 0x0000ff00ul);
          }
        } else if (kBlendingMode == kColorize) {
          Ui32 ca = (Ui32(color.a) * (Ui32(in_color.a) + 1u)) >> 8u;
          if (ca == 255) {
            Ui32 r2 = (Ui32(color.r) * (Ui32(in_color.r) + 1)) >> 8u;
            Ui32 g2 = (Ui32(color.g) * (Ui32(in_color.g) + 1)) >> 8u;
            Ui32 b2 = (Ui32(color.b) * (Ui32(in_color.b) + 1)) >> 8u;
            to_rgba->rgba = Rgba((Ui8)r2, (Ui8)g2, (Ui8)b2).rgba;
          } else if (ca) {
            Ui32 m = 255 - ca;
            Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
            Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8u) * m;

            Ui32 m2 = ca;
            Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
            Ui32 g2 = (Ui32(color.g) * m2 * (Ui32(in_color.g) + 1)) >> 8u;
            Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
            Ui32 rb2 = ((r2 >> 8u) & 0xff00u) + (b2 & 0xff000000);

            to_rgba->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) |
              ((g + g2) & 0x0000ff00ul);
          }
        } else {  // Unknown blending mode!
          to_rgba->rgba = color.rgba;
        }
      }
    }
    return;
  }

  const Si32 to_x = to_x_pivot -
    from_sprite.Pivot().x * to_width / from_width;
  const Si32 to_y = to_y_pivot -
    from_sprite.Pivot().y * to_height / from_height;

  Rgba *to = to_sprite->RgbaData()
    + to_y * to_stride_pixels
    + to_x;
  const Rgba *from = from_sprite.RgbaData()
    + from_y * from_stride_pixels
    + from_x;

  const Si32 to_y_db = (to_y >= 0 ? 0 : -to_y);
  const Si32 to_y_d_max = to_sprite->Height() - to_y;
  const Si32 to_y_de = (to_height < to_y_d_max ? to_height : to_y_d_max);

  const Si32 to_x_db = (to_x >= 0 ? 0 : -to_x);
  const Si32 to_x_d_max = to_sprite->Width() - to_x;
  const Si32 to_x_de = (to_width < to_x_d_max ? to_width : to_x_d_max);

  const Si32 from_y_step_16 = 65536 * from_height / to_height;
  Si32 from_y_disp_0 = ((from_height * to_y_db) / to_height);
  Si32 from_y_acc_16 = 0;
  if (kFilterMode == kFilterBilinear) {
    from_y_acc_16 = -32767;
  }
  Ui32 from_y_8 = 0;
  for (Si32 to_y_disp = to_y_db; to_y_disp < to_y_de; ++to_y_disp) {
    const Si32 from_x_b = (from_width * to_x_db) / to_width;
    const Si32 from_x_step_16 = 65536 * from_width / to_width;
    Si32 from_x_acc_16 = 0;
    if (kFilterMode == kFilterBilinear) {
      from_x_acc_16 = -32767;
    }

    Si32 from_y_disp = from_y_disp_0 + (from_y_acc_16 / 65536);
    from_y_acc_16 += from_y_step_16;

    const Rgba *from_line_0 = from + from_y_disp * from_stride_pixels;
    const Rgba *from_line_1 = from +
      std::min(from_height - 1, from_y_disp + 1) * from_stride_pixels;
    Rgba *to_line = to + to_y_disp * to_stride_pixels;

    Ui32 from_x_8 = 0;
    Si32 from_x_disp_00 = from_x_b;
    Si32 from_x_disp_01 = from_x_b + 1;
    Rgba *to_rgba = to_line + to_x_db;
    for (Si32 to_x_disp = to_x_db; to_x_disp < to_x_de; ++to_x_disp) {
      from_x_acc_16 += from_x_step_16;

      Rgba color;
      if (kFilterMode == kFilterNearest) {
        color = *(from_line_0 + from_x_disp_00);
      } else if (kFilterMode == kFilterBilinear) {
        Rgba color00 = *(from_line_0 + from_x_disp_00);
        Rgba color01 = *(from_line_0 + from_x_disp_00 + 1);
        Rgba color10 = *(from_line_1 + from_x_disp_00);
        Rgba color11 = *(from_line_1 + from_x_disp_00 + 1);
        color = Rgba(
           (Ui8)(((Ui32(color00.r) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.r) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.r) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.r) * ((from_x_8) * (from_y_8)))) >> 16u),
           (Ui8)(((Ui32(color00.g) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.g) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.g) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.g) * ((from_x_8) * (from_y_8)))) >> 16u),
           (Ui8)(((Ui32(color00.b) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.b) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.b) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.b) * ((from_x_8) * (from_y_8)))) >> 16u),
           (Ui8)(((Ui32(color00.a) * ((255 - from_x_8) * (255 - from_y_8))) +
            (Ui32(color01.a) * ((from_x_8) * (255 - from_y_8))) +
            (Ui32(color10.a) * ((255 - from_x_8) * (from_y_8))) +
            (Ui32(color11.a) * ((from_x_8) * (from_y_8)))) >> 16u));
      }


      if (kBlendingMode == kCopyRgba) {
        to_rgba->rgba = color.rgba;
      } else if (kBlendingMode == kAlphaBlend) {
        if (color.a == 255) {
          to_rgba->rgba = color.rgba;
        } else if (color.a) {
          Ui32 m = 255 - color.a;
          Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
          Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8u) * m;
          Ui32 m2 = color.a;
          Ui32 rb2 = (color.rgba & 0x00ff00fful) * m2;
          Ui32 g2 = ((color.rgba & 0x0000ff00ul) >> 8u) * m2;
          to_rgba->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) |
            ((g + g2) & 0x0000ff00ul);
        }
      } else if (kBlendingMode == kColorize) {
        Ui32 ca = (Ui32(color.a) * (Ui32(in_color.a) + 1)) >> 8u;
        if (ca == 255) {
          Ui32 r2 = (Ui32(color.r) * (Ui32(in_color.r) + 1)) >> 8u;
          Ui32 g2 = (Ui32(color.g) * (Ui32(in_color.g) + 1)) & 0xff00u;
          Ui32 b2 = ((Ui32(color.b) * (Ui32(in_color.b) + 1)) << 8u) & 0xff0000ull;
          to_rgba->rgba = r2 | g2 | b2;
        } else if (ca) {
          Ui32 m = 255 - ca;
          Ui32 rb = (to_rgba->rgba & 0x00ff00fful) * m;
          Ui32 g = ((to_rgba->rgba & 0x0000ff00ul) >> 8u) * m;

          Ui32 m2 = ca;
          Ui32 r2 = Ui32(color.r) * m2 * (Ui32(in_color.r) + 1);
          Ui32 g2 = (Ui32(color.g) * m2 * (Ui32(in_color.g) + 1)) >> 8u;
          Ui32 b2 = Ui32(color.b) * m2 * (Ui32(in_color.b) + 1);
          Ui32 rb2 = ((r2 >> 8u) & 0xff00u) + (b2 & 0xff000000ull);

          to_rgba->rgba = (((rb + rb2) >> 8u) & 0x00ff00fful) |
          ((g + g2) & 0x0000ff00ul);
        }
      } else {  // Unknown blending mode!
        to_rgba->rgba = color.rgba;
      }

      if (from_x_acc_16 > 0) {
        from_x_8 = (static_cast<Ui32>(from_x_acc_16) & 65535ul) >> 8u;
        from_x_disp_00 = from_x_b + static_cast<Si32>((static_cast<Ui32>(from_x_acc_16) >> 16u));
        from_x_disp_01 = from_x_disp_00 + 1;
      }
      ++to_rgba;
    }
    if (from_y_acc_16 > 0) {
      from_y_8 = (static_cast<Ui32>(from_y_acc_16) & 65535ul) >> 8u;
    }
  }
}

template void DrawSprite<kCopyRgba, kFilterNearest>(
  Sprite *to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kAlphaBlend, kFilterNearest>(
  Sprite *to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kColorize, kFilterNearest>(
  Sprite *to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kCopyRgba, kFilterBilinear>(
  Sprite *to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kAlphaBlend, kFilterBilinear>(
  Sprite *to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);
template void DrawSprite<kColorize, kFilterBilinear>(
  Sprite *to_sprite, const Si32 to_x_pivot, const Si32 to_y_pivot,
  const Si32 to_width, const Si32 to_height,
  const Sprite &from_sprite, const Si32 from_x, const Si32 from_y,
  const Si32 from_width, const Si32 from_height,
  Rgba color);


Sprite::Sprite() {
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(0, 0);
  pivot_ = Vec2Si32(0, 0);
}

void Sprite::LoadFromData(const Ui8* data, Ui64 size_bytes,
    const char *file_name) {
  if (!file_name) {
    *Log() << "Error in Sprite::Load, file_name is nullptr."
      " Not loading sprite.";
    return;
  }
  if (data == nullptr) {
    *Log() << "Error in Sprite::Load, file: \""
      << file_name << "\" could not be loaded, data=nullptr."
      " Not loading sprite.";
    return;
  }
  const char *last_dot = strchr(file_name, '.');
  if (!last_dot) {
    *Log() << "Error in Sprite::Load, file: \""
      << file_name << "\" has no extension."
      " Not loading sprite.";
    return;
  }
  if (strcmp(last_dot, ".tga") == 0) {
    if (size_bytes == 0) {
      *Log() << "Error in Sprite::Load, file: \""
        << file_name << "\" could not be loaded (size=0)."
          " Not loading sprite.";
      return;
    }
    sprite_instance_ = LoadTga(data, static_cast<Si64>(size_bytes));
    if (!sprite_instance_) {
      *Log() << "Error in Sprite::Load, file: \""
        << file_name << "\" could not be loaded with LoadTga."
          " Not loading sprite.";
      return;
    }
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(sprite_instance_->width(),
                         sprite_instance_->height());
    pivot_ = Vec2Si32(0, 0);
  } else {
    *Log() << "Error in Sprite::Load, file: \""
      << file_name << "\" could not be loaded,"
        " unknown file extension: \"" << last_dot << "\"."
        " Not loading sprite.";
    return;
  }
  UpdateOpaqueSpans();
}

void Sprite::Load(const char *file_name) {
  if (!file_name) {
    *Log() << "Error in Sprite::Load, file_name is nullptr."
      " Not loading sprite.";
    return;
  }
  const char *last_dot = strchr(file_name, '.');
  if (!last_dot) {
    *Log() << "Error in Sprite::Load, file: \""
      << file_name << "\" has no extension."
      " Not loading sprite.";
    return;
  }
  if (strcmp(last_dot, ".tga") == 0) {
    std::vector<Ui8> data = ReadFile(file_name, true);
    if (data.empty()) {
      *Log() << "Error in Sprite::Load, file: \""
        << file_name << "\" could not be loaded (data is empty)."
          " Not loading sprite.";
      return;
    }
    sprite_instance_ = LoadTga(data.data(), static_cast<Si64>(data.size()));
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = sprite_instance_ ? Vec2Si32(sprite_instance_->width(),
      sprite_instance_->height()) : Vec2Si32(0, 0);
    pivot_ = Vec2Si32(0, 0);
  } else {
    *Log() << "Error in Sprite::Load, file: \""
      << file_name << "\" could not be loaded,"
        " unknown file extension: \"" << last_dot << "\"."
        " Not loading sprite.";
    return;
  }
  UpdateOpaqueSpans();
}

void Sprite::Load(const std::string &file_name) {
  Load(file_name.c_str());
}

void Sprite::Save(const char *file_name) {
  std::vector<Ui8> data = SaveToData(file_name);
  if (!data.empty()) {
    WriteFile(file_name, data.data(), data.size());
  }
}

void Sprite::Save(const std::string &file_name) {
  Save(file_name.c_str());
}

std::vector<Ui8> Sprite::SaveToData(const char *file_name) {
  std::vector<Ui8> data;
  Check(!!file_name, "Error in Sprite::Save, file_name is nullptr.");
  const char *last_dot = strchr(file_name, '.');
  Check(!!last_dot, "Error in Sprite::Save, file_name has no extension.");
  if (strcmp(last_dot, ".tga") == 0) {
    SaveTga(sprite_instance_, &data);
  } else {
    Fatal("Error in Sprite::Save, unknown file extension.");
  }
  return data;
}

void Sprite::Create(const Vec2Si32 size) {
  Create(size.x, size.y);
}

void Sprite::Create(const Si32 width, const Si32 height) {
  sprite_instance_ = std::make_shared<SpriteInstance>(width, height);
  ref_pos_ = Vec2Si32(0, 0);
  ref_size_ = Vec2Si32(width, height);
  pivot_ = Vec2Si32(0, 0);
  Clear();
}

void Sprite::Reference(const Sprite &from, const Si32 from_x, const Si32 from_y,
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
  if (!sprite_instance_) {
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
  if (!sprite_instance_) {
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

void Sprite::Clone(Sprite from, CloneTransform transform) {
  if (!from.sprite_instance_) {
    sprite_instance_ = nullptr;
    ref_pos_ = Vec2Si32(0, 0);
    ref_size_ = Vec2Si32(0, 0);
    pivot_ = Vec2Si32(0, 0);
    return;
  }
  if (transform == kCloneUntransformed) {
    Create(from.Width(), from.Height());
    from.Draw(from.Pivot().x, from.Pivot().y, from.Width(), from.Height(),
      0, 0, from.Width(), from.Height(), *this, kCopyRgba);
    SetPivot(from.Pivot());
    UpdateOpaqueSpans();
    return;
  }
  Vec2Si32 dst_base;
  Vec2Si32 dst_dir_x;
  Vec2Si32 dst_dir_y;
  if (transform == kCloneRotateCw90 || transform == kCloneRotateCcw90) {
    Create(from.Height(), from.Width());
    if (transform == kCloneRotateCw90) {
      dst_base = Vec2Si32(0, Height() - 1);
      dst_dir_x = Vec2Si32(0, -1);
      dst_dir_y = Vec2Si32(1, 0);
    } else {
      dst_base = Vec2Si32(Width() - 1, 0);
      dst_dir_x = Vec2Si32(0, 1);
      dst_dir_y = Vec2Si32(-1, 0);
    }
  } else {
    Create(from.Width(), from.Height());
    if (transform == kCloneMirrorLr) {
      dst_base = Vec2Si32(Width() - 1, 0);
      dst_dir_x = Vec2Si32(-1, 0);
      dst_dir_y = Vec2Si32(0, 1);
    } else if (transform == kCloneMirrorUd) {
      dst_base = Vec2Si32(0, Height() - 1);
      dst_dir_x = Vec2Si32(1, 0);
      dst_dir_y = Vec2Si32(0, -1);
    } else {  // kCloneRotate180
      dst_base = Vec2Si32(Width() - 1, Height() - 1);
      dst_dir_x = Vec2Si32(-1, 0);
      dst_dir_y = Vec2Si32(0, -1);
    }
  }

  Si32 wid = from.Width();
  Si32 hei = from.Height();
  Si32 src_stride = from.StridePixels();
  Si32 dst_stride = StridePixels();
  Rgba *src_data = from.RgbaData();
  Rgba *dst_data = RgbaData();
  for (Si32 y = 0; y < hei; ++y) {
    for (Si32 x = 0; x < wid; ++x) {
      Vec2Si32 dst_pos = dst_base + dst_dir_y * y + dst_dir_x * x;
      dst_data[dst_pos.y * dst_stride + dst_pos.x] =
        src_data[y * src_stride + x];
    }
  }

  SetPivot(dst_base + from.Pivot().x * dst_dir_x + from.Pivot().y * dst_dir_y);
}

void Sprite::SetPivot(Vec2Si32 pivot) {
  pivot_ = pivot;
}

Vec2Si32 Sprite::Pivot() const {
  return pivot_;
}

void Sprite::Draw(Sprite to_sprite,
    const Si32 to_x_pivot, const Si32 to_y_pivot,
  DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba color) {
  if (!sprite_instance_) {
    return;
  }
  switch (filter_mode) {
    case kFilterNearest:
      switch (blending_mode) {
        case kAlphaBlend:
          DrawSprite<kAlphaBlend, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
        case kCopyRgba:
          DrawSprite<kCopyRgba, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
        case kColorize:
          DrawSprite<kColorize, kFilterNearest>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(),
            color);
          break;
      }
      break;
    case kFilterBilinear:
      switch (blending_mode) {
        case kAlphaBlend:
          DrawSprite<kAlphaBlend, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
        case kCopyRgba:
          DrawSprite<kCopyRgba, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
        case kColorize:
          DrawSprite<kColorize, kFilterBilinear>(&to_sprite,
            to_x_pivot, to_y_pivot,
            Width(), Height(),
            *this, 0, 0, Width(), Height(), color);
          break;
      }
      break;
  }
}


void Sprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba color) {
  Draw(GetEngine()->GetBackbuffer(), to_x_pivot, to_y_pivot,
       blending_mode, filter_mode, color);
}

void Sprite::Draw(const Vec2Si32 to, float angle_radians,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to.x, to.y, angle_radians, 1.f, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y, float angle_radians,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_x, to_y, angle_radians, 1.f, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Vec2Si32 to, float angle_radians, float zoom,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to.x, to.y, angle_radians, zoom, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    float angle_radians, float zoom,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_x, to_y, angle_radians, zoom, GetEngine()->GetBackbuffer(),
      blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    float angle_radians, float zoom, Sprite to_sprite,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  if (!sprite_instance_) {
    return;
  }
  Vec2F pivot = Vec2F(Vec2Si32(to_x, to_y));
  float sin_a = sinf(angle_radians) * zoom;
  float cos_a = cosf(angle_radians) * zoom;
  Vec2F left = Vec2F(-cos_a, -sin_a) * static_cast<float>(pivot_.x);
  Vec2F right = Vec2F(cos_a, sin_a) * static_cast<float>(Width() - 1 - pivot_.x);
  Vec2F up = Vec2F(-sin_a, cos_a) * static_cast<float>(Height() - 1 - pivot_.y);
  Vec2F down = Vec2F(sin_a, -cos_a) * static_cast<float>(pivot_.y);

  // d c
  // a b
  Vec2F a(pivot + left + down);
  Vec2F b(pivot + right + down);
  Vec2F c(pivot + right + up);
  Vec2F d(pivot + left + up);

  Vec2F ta(0.01f,
    0.01f);
  Vec2F tb(static_cast<float>(ref_size_.x) - 1.01f,
    0.01f);
  Vec2F tc(static_cast<float>(ref_size_.x) - 1.01f,
    static_cast<float>(ref_size_.y) - 1.01f);
  Vec2F td(0.01f,
    static_cast<float>(ref_size_.y) - 1.01f);

  switch (filter_mode) {
    case kFilterNearest:
      switch (blending_mode) {
        case kCopyRgba:
          DrawTriangle<kCopyRgba, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kCopyRgba, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kAlphaBlend:
          DrawTriangle<kAlphaBlend, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kAlphaBlend, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kColorize:
          DrawTriangle<kColorize, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kColorize, kFilterNearest>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
      }
      break;
    case kFilterBilinear:
      switch (blending_mode) {
        case kCopyRgba:
          DrawTriangle<kCopyRgba, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kCopyRgba, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kAlphaBlend:
          DrawTriangle<kAlphaBlend, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kAlphaBlend, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
        case kColorize:
          DrawTriangle<kColorize, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, *this, in_color);
          DrawTriangle<kColorize, kFilterBilinear>(to_sprite,
            d, a, c, td, ta, tc, *this, in_color);
          break;
      }

      break;
  }
}

void DrawTriangle(Sprite to_sprite,
    Vec2F a, Vec2F b, Vec2F c,
    Vec2F ta, Vec2F tb, Vec2F tc,
    Sprite texture,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  switch (filter_mode) {
    case kFilterNearest:
      switch (blending_mode) {
        case kCopyRgba:
          DrawTriangle<kCopyRgba, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kAlphaBlend:
          DrawTriangle<kAlphaBlend, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kColorize:
          DrawTriangle<kColorize, kFilterNearest>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
      }
      break;
    case kFilterBilinear:
      switch (blending_mode) {
        case kCopyRgba:
          DrawTriangle<kCopyRgba, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kAlphaBlend:
          DrawTriangle<kAlphaBlend, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
        case kColorize:
          DrawTriangle<kColorize, kFilterBilinear>(to_sprite,
            a, b, c, ta, tb, tc, texture, in_color);
          break;
      }

      break;
  }
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
    Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
    0, 0, ref_size_.x, ref_size_.y, blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Si32 to_x, const Si32 to_y,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
    from_x, from_y, from_width, from_height,
    GetEngine()->GetBackbuffer(), blending_mode, filter_mode, in_color);
}

void Sprite::Draw(Sprite to_sprite, const Si32 to_x, const Si32 to_y,
                  const Si32 to_width, const Si32 to_height,
                  const Si32 from_x, const Si32 from_y,
                  const Si32 from_width, const Si32 from_height,
                  DrawBlendingMode blending_mode, DrawFilterMode filter_mode,
                  Rgba in_color) {
  Draw(to_x, to_y, to_width, to_height,
       from_x, from_y, from_width, from_height,
       to_sprite, blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Vec2Si32 to_pos, DrawBlendingMode blending_mode,
    DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, blending_mode, filter_mode, in_color);
}

void Sprite::Draw(Sprite to_sprite, const Vec2Si32 to_pos,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_sprite, to_pos.x, to_pos.y, blending_mode, filter_mode, in_color);
}

void Sprite::Draw(Sprite to_sprite, const Vec2Si32 to_pos,
    const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_sprite, to_pos.x, to_pos.y, to_size.x, to_size.y,
       0, 0, ref_size_.x, ref_size_.y,
       blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
    0, 0, ref_size_.x, ref_size_.y,
    blending_mode, filter_mode, in_color);
}

void Sprite::Draw(const Vec2Si32 to_pos, const Vec2Si32 to_size,
    const Vec2Si32 from_pos, const Vec2Si32 from_size,
    DrawBlendingMode blending_mode, DrawFilterMode filter_mode, Rgba in_color) {
  Draw(to_pos.x, to_pos.y, to_size.x, to_size.y,
    from_pos.x, from_pos.y, from_size.x, from_size.y,
    blending_mode, filter_mode, in_color);
}


void Sprite::Draw(const Si32 to_x_pivot, const Si32 to_y_pivot,
    const Si32 to_width, const Si32 to_height,
    const Si32 from_x, const Si32 from_y,
    const Si32 from_width, const Si32 from_height,
    Sprite to_sprite, DrawBlendingMode blending_mode,
    DrawFilterMode filter_mode, Rgba in_color) const {
  if (!sprite_instance_) {
    return;
  }
  switch (filter_mode) {
      case kFilterNearest:
      switch (blending_mode) {
        default:
        case kCopyRgba:
          DrawSprite<kCopyRgba, kFilterNearest>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kAlphaBlend:
          DrawSprite<kAlphaBlend, kFilterNearest>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kColorize:
          DrawSprite<kColorize, kFilterNearest>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
      }
      break;
      case kFilterBilinear:
      switch (blending_mode) {
        default:
        case kCopyRgba:
          DrawSprite<kCopyRgba, kFilterBilinear>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kAlphaBlend:
          DrawSprite<kAlphaBlend, kFilterBilinear>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
        case kColorize:
          DrawSprite<kColorize, kFilterBilinear>(&to_sprite,
              to_x_pivot, to_y_pivot, to_width, to_height,
              *this, from_x, from_y, from_width, from_height,
              in_color);
          break;
      }
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
  return sprite_instance_->width() * static_cast<Si32>(sizeof(Rgba));
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

const Rgba* Sprite::RgbaData() const {
  return (static_cast<Rgba*>(static_cast<void*>(
      sprite_instance_->RawData())) +
    ref_pos_.y * StridePixels() +
    ref_pos_.x);
}

const std::vector<SpanSi32> &Sprite::Opaque() const {
  return sprite_instance_->Opaque();
}

void Sprite::UpdateOpaqueSpans() {
  if (sprite_instance_) {
    sprite_instance_->UpdateOpaqueSpans();
  }
}

void Sprite::ClearOpaqueSpans() {
  if (sprite_instance_) {
    sprite_instance_->ClearOpaqueSpans();
  }
}

}  // namespace arctic
