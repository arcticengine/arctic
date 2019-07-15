// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
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

#ifndef ENGINE_RGBA_H_
#define ENGINE_RGBA_H_

#include "engine/arctic_types.h"

#define MASK_LO    0x00ff00ff
#define MASK_HI    0xff00ff00
#define MASK_BLEND 0xfefefeff

namespace arctic {

struct Rgba {
  union {
    struct {
      Ui8 r;
      Ui8 g;
      Ui8 b;
      Ui8 a;
    };
    Ui32 rgba;
    Ui8 element[4];
  };

  Rgba() {}

  explicit Rgba(Ui8 r_in, Ui8 g_in, Ui8 b_in) {
    r = r_in;
    g = g_in;
    b = b_in;
    a = 255;
  }
  // a = 255 is opaque, a = 0 is transparent.
  explicit Rgba(Ui8 r_in, Ui8 g_in, Ui8 b_in, Ui8 a_in) {
    r = r_in;
    g = g_in;
    b = b_in;
    a = a_in;
  }
  explicit Rgba(Ui8 s) {
    r = s;
    g = s;
    b = s;
    a = 255;
  }

  /// rgba_in is 32 bits containing 0xAABBGGRR. 0xff00ff00 is opaque green.
  explicit Rgba(Ui32 rgba_in) {
    rgba = rgba_in;
  }
  Ui8 &operator[](Si32 i) {
    return element[i];
  }
  const Ui8 &operator[](Si32 i) const {
    return element[i];
  }

  Rgba &operator =(const Rgba &v) {
    rgba = v.rgba;
    return *this;
  }

  const bool operator== (const Rgba &v) const {
    return rgba == v.rgba;
  }
  const bool operator!= (const Rgba &v) const {
    return rgba != v.rgba;
  }
};


inline Rgba Mix(Rgba const a, Rgba const b, float const f) {
  return Rgba(static_cast<Ui8>(a.r * (1.0f - f) + f * b.r),
              static_cast<Ui8>(a.g * (1.0f - f) + f * b.g),
              static_cast<Ui8>(a.b * (1.0f - f) + f * b.b),
              static_cast<Ui8>(a.a * (1.0f - f) + f * b.a));
}
inline Rgba Min(const Rgba a, const Rgba b) {
  return Rgba((a.r < b.r) ? a.r : b.r,
              (a.g < b.g) ? a.g : b.g,
              (a.b < b.b) ? a.b : b.b,
              (a.a < b.a) ? a.a : b.a);
}
inline Rgba Max(const Rgba a, const Rgba b) {
  return Rgba((a.r > b.r) ? a.r : b.r,
              (a.g > b.g) ? a.g : b.g,
              (a.b > b.b) ? a.b : b.b,
              (a.a > b.a) ? a.a : b.a);
}
inline Rgba Clamp(const Rgba rgba, const Rgba mi, const Rgba ma) {
  return Max(Min(rgba, ma), mi);
}

inline Rgba BlendFast(Rgba c1, Rgba c2) {
  return Rgba(static_cast<Ui32>((
      (static_cast<Ui64>(c1.rgba) & MASK_BLEND) +
      (static_cast<Ui64>(c2.rgba) & MASK_BLEND)) >> 1));
}

inline Rgba Mix(Rgba c1, Rgba c2, Ui32 alpha_1_8) {
  Ui32 a = c1.rgba & MASK_LO;
  Ui32 b = c2.rgba & MASK_LO;
  Ui32 d = a + (((b - a) * alpha_1_8) >> 8);
  d = d & MASK_LO;

  Ui32 m = (c1.rgba & MASK_HI) >> 8;
  Ui32 n = (c2.rgba & MASK_HI) >> 8;
  Ui32 e = (c1.rgba & MASK_HI) + ((n - m) * alpha_1_8);
  e = e & MASK_HI;

  return Rgba(d | e);
}

inline Rgba Scale(Rgba c, Ui32 alpha_1_8) {
  Ui32 a = c.rgba & MASK_LO;
  Ui32 d = (a * alpha_1_8) >> 8;
  d = d & MASK_LO;

  a = (c.rgba & MASK_HI) >> 8;
  Ui32 e = a * alpha_1_8;
  e = e & MASK_HI;

  return Rgba(d | e);
}

inline Rgba Lerp(Rgba c1, Rgba c2, Si32 alpha_1_8) {
  Ui32 a = c1.rgba & MASK_LO;
  Ui32 b = c2.rgba & MASK_LO;
  Ui32 d = a + (((b - a) * alpha_1_8) >> 8);
  d = d & MASK_LO;

  a = (c1.rgba & MASK_HI) >> 8;
  b = (c2.rgba & MASK_HI) >> 8;
  Ui32 e = (c1.rgba & MASK_HI) + ((b - a) * alpha_1_8);
  e = e & MASK_HI;

  return Rgba(d | e);
}

inline Rgba GetGray(Rgba c) {
  Ui32 res = 19595 * static_cast<Ui32>(c.r)
    + 38470 * static_cast<Ui32>(c.g)
    + 7471 * static_cast<Ui32>(c.b);
  return Rgba(res >> 16);
}

//    ^y
// 256+ c      d
//    |
//  ay+.....P
//    |     .
//   0+ a   .  b
//    +-+---+--+-->x
//      0   ax 256
// Calculate color at point P
inline Rgba Bilerp(Rgba a, Rgba b, Rgba c, Rgba d, Si32 ax, Si32 ay) {
  const Si32 axy = (ax * ay) >> 8;
  Ui32 aa = a.rgba & MASK_LO;
  Ui32 bb = b.rgba & MASK_LO;
  Ui32 cc = c.rgba & MASK_LO;
  Ui32 dd = d.rgba & MASK_LO;

  Ui32 rb = (aa + ((
      (bb - aa) * ax + (cc - aa) * ay + (aa + dd - bb - cc) * axy) >> 8))
    & MASK_LO;

  aa = (a.rgba & MASK_HI) >> 8;
  bb = (b.rgba & MASK_HI) >> 8;
  cc = (c.rgba & MASK_HI) >> 8;
  dd = (d.rgba & MASK_HI) >> 8;

  Ui32 gg = ((a.rgba & MASK_HI) + (
      (bb - aa) * ax + (cc - aa) * ay + (aa + dd - bb - cc) * axy))
    & MASK_HI;

  return Rgba(rb | gg);
}

}  // namespace arctic

#undef MASK_BLEND
#undef MASK_HI
#undef MASK_LO

#endif  // ENGINE_RGBA_H_
