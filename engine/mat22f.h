// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 Huldra
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

#ifndef ENGINE_MAT22F_H_
#define ENGINE_MAT22F_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {
/// @addtogroup global_math
/// @{
struct Mat22F {
  float m[4];

  Mat22F() {}

  explicit Mat22F(float a0, float a1,
    float a2, float a3) {
    m[0] = a0;
    m[1] = a1;
    m[2] = a2;
    m[3] = a3;
  }

  float &operator[](Si32 i) {
    return m[i];
  }
  const float &operator[](Si32 i) const {
    return m[i];
  }
};

inline Vec2F operator*(Mat22F const &m, Vec2F const &v) {
  return Vec2F(v.x * m[0] + v.y * m[1],
    v.x * m[2] + v.y * m[3]);
}

inline float Determinant(Mat22F const &m) {
  return m.m[0] * m.m[3] - m.m[1] * m.m[2];
}

inline Mat22F Rotation(const float t) {
  const float co = cosf(t);
  const float si = sinf(t);

  return Mat22F(co, -si,
    si, co);
}

inline Mat22F Rotation(Vec2F const &v) {
  return Mat22F(v.x, -v.y,
    v.y, v.x);
}
/// @}

}  // namespace arctic

#endif  // ENGINE_MAT22F_H_
