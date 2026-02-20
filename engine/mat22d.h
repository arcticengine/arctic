// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 Huldra
// Copyright (c) 2020 The Lasting Curator
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

#ifndef ENGINE_MAT22D_H_
#define ENGINE_MAT22D_H_

#include <cmath>
#include "engine/arctic_types.h"
#include "engine/vec2d.h"

namespace arctic {
/// @addtogroup global_math
/// @{
struct Mat22D {
  double m[4];

  Mat22D() {}

  explicit Mat22D(double a0, double a1,
    double a2, double a3) {
    m[0] = a0;
    m[1] = a1;
    m[2] = a2;
    m[3] = a3;
  }

  double &operator[](Si32 i) {
    return m[i];
  }
  const double &operator[](Si32 i) const {
    return m[i];
  }
};

inline Vec2D operator*(Mat22D const &m, Vec2D const &v) {
  return Vec2D(v.x * m[0] + v.y * m[1],
    v.x * m[2] + v.y * m[3]);
}

inline double Determinant(Mat22D const &m) {
  return m.m[0] * m.m[3] - m.m[1] * m.m[2];
}

inline Mat22D Rotation(const double t) {
  const double co = std::cos(t);
  const double si = std::sin(t);

  return Mat22D(co, -si,
    si, co);
}

inline Mat22D Rotation(Vec2D const &v) {
  return Mat22D(v.x, -v.y,
    v.y, v.x);
}
/// @}

}  // namespace arctic

#endif  // ENGINE_MAT22D_H_
