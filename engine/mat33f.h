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

#ifndef ENGINE_MAT33F_H_
#define ENGINE_MAT33F_H_

#define _USE_MATH_DEFINES
#include <cmath>
#include "engine/arctic_types.h"
#include "engine/vec3f.h"

namespace arctic {
/// @addtogroup global_math
/// @{
struct Mat33F {
  float m[9];

  Mat33F() {}

  explicit Mat33F(float a0, float a1, float a2,
    float a3, float a4, float a5,
    float a6, float a7, float a8) {
    m[0] = a0;
    m[1] = a1;
    m[2] = a2;
    m[3] = a3;
    m[4] = a4;
    m[5] = a5;
    m[6] = a6;
    m[7] = a7;
    m[8] = a8;
  }

  explicit Mat33F(const Vec3F &a, const Vec3F &b, const Vec3F &c) {
    m[0] = a.x;
    m[1] = b.x;
    m[2] = c.x;
    m[3] = a.y;
    m[4] = b.y;
    m[5] = c.y;
    m[6] = a.z;
    m[7] = b.z;
    m[8] = c.z;
  }

  float &operator[](Si32 i) {
    return m[i];
  }
  const float &operator[](Si32 i) const {
    return m[i];
  }

  Vec3F x() const {
    return Vec3F(m[0], m[3], m[6]);
  }
  Vec3F y() const {
    return Vec3F(m[1], m[4], m[7]);
  }
  Vec3F z() const {
    return Vec3F(m[2], m[5], m[8]);
  }
};

inline Mat33F operator*(Mat33F const &a, Mat33F const &b) {
  return Mat33F(a[0] * b[0] + a[1] * b[3] + a[2] * b[6],
    a[0] * b[1] + a[1] * b[4] + a[2] * b[7],
    a[0] * b[2] + a[1] * b[5] + a[2] * b[8],

    a[3] * b[0] + a[4] * b[3] + a[5] * b[6],
    a[3] * b[1] + a[4] * b[4] + a[5] * b[7],
    a[3] * b[2] + a[4] * b[5] + a[5] * b[8],

    a[6] * b[0] + a[7] * b[3] + a[8] * b[6],
    a[6] * b[1] + a[7] * b[4] + a[8] * b[7],
    a[6] * b[2] + a[7] * b[5] + a[8] * b[8]);
}

inline Vec3F operator*(Mat33F const &m, Vec3F const &v) {
  return Vec3F(v.x * m[0] + v.y * m[1] + v.z * m[2],
    v.x * m[3] + v.y * m[4] + v.z * m[5],
    v.x * m[6] + v.y * m[7] + v.z * m[8]);
}

inline float Determinant(Mat33F const &m) {
  return m.m[0] * m.m[4] * m.m[8]
    + m.m[3] * m.m[7] * m.m[2]
    + m.m[1] * m.m[5] * m.m[6]
    - m.m[2] * m.m[4] * m.m[6]
    - m.m[1] * m.m[3] * m.m[8]
    - m.m[5] * m.m[7] * m.m[0];
}

inline Mat33F SetScale3(const Vec3F &s) {
  return Mat33F(s.x, 0.0f, 0.0f,
    0.0f, s.y, 0.0f,
    0.0f, 0.0f, s.z);
}


inline Mat33F Transpose(Mat33F const &m) {
  return Mat33F(m.m[0], m.m[3], m.m[6],
    m.m[1], m.m[4], m.m[7],
    m.m[2], m.m[5], m.m[8]);
}

inline Vec3F Rotate(const Vec3F &v, float t, const Vec3F &a) {
  const float sint = sinf(t);
  const float cost = cosf(t);
  const float icost = 1.0f - cost;

  const Mat33F m = Mat33F(a.x * a.x * icost + cost,
    a.y * a.x * icost - sint * a.z,
    a.z * a.x * icost + sint * a.y,

    a.x * a.y * icost + sint * a.z,
    a.y * a.y * icost + cost,
    a.z * a.y * icost - sint * a.x,

    a.x * a.z * icost - sint * a.y,
    a.y * a.z * icost + sint * a.x,
    a.z * a.z * icost + cost);
  return m * v;
}

inline Vec3F Rotate(const Vec3F &v, float cost, float sint, const Vec3F &a) {
  const float icost = 1.0f - cost;

  const Mat33F m = Mat33F(a.x * a.x * icost + cost,
    a.y * a.x * icost - sint * a.z,
    a.z * a.x * icost + sint * a.y,

    a.x * a.y * icost + sint * a.z,
    a.y * a.y * icost + cost,
    a.z * a.y * icost - sint * a.x,

    a.x * a.z * icost - sint * a.y,
    a.y * a.z * icost + sint * a.x,
    a.z * a.z * icost + cost);
  return m * v;
}

inline Vec3F RotateX(const Vec3F &v, float a) {
  const float si = sinf(a);
  const float co = cosf(a);

  return Vec3F(v[0], v[1] * co - v[2] * si, v[1] * si + v[2] * co);
}

inline Vec3F RotateY(const Vec3F &v, float a) {
  const float si = sinf(a);
  const float co = cosf(a);

  return Vec3F(v[0] * co + v[2] * si, v[1], -v[0] * si + v[2] * co);
}

inline Vec3F RotateZ(const Vec3F &v, float a) {
  const float si = sinf(a);
  const float co = cosf(a);

  return Vec3F(v[0] * co + v[1] * si, -v[0] * si + v[1] * co, v[2]);
}

inline Mat33F SetRotationAxisAngle3(const Vec3F &a, const float t) {
  const float sint = sinf(t);
  const float cost = cosf(t);
  const float icost = 1.0f - cost;

  return Mat33F(a.x * a.x * icost + cost,
    a.y * a.x * icost - sint * a.z,
    a.z * a.x * icost + sint * a.y,

    a.x * a.y * icost + sint * a.z,
    a.y * a.y * icost + cost,
    a.z * a.y * icost - sint * a.x,

    a.x * a.z * icost - sint * a.y,
    a.y * a.z * icost + sint * a.x,
    a.z * a.z * icost + cost);
}


inline Mat33F SetRotationEuler3(float x, float y, float z) {
  const float a = sinf(x);
  const float b = cosf(x);
  const float c = sinf(y);
  const float d = cosf(y);
  const float e = sinf(z);
  const float f = cosf(z);
  const float ac = a * c;
  const float bc = b * c;

  return Mat33F(d * f, d * e, -c,
    ac * f - b * e, ac * e + b * f, a * d,
    bc * f + a * e, bc * e - a * f, b * d);
}


inline Mat33F SetRotationEuler3(const Vec3F &xyz) {
  return SetRotationEuler3(xyz.x, xyz.y, xyz.z);
}

inline Mat33F SetIdentity3(void) {
  return Mat33F(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}


inline Mat33F BuildBase(const Vec3F &nn) {
  Vec3F up;
  if (fabsf(nn.z) < 0.9f) {
    up.x = 0.0f;
    up.y = 0.0f;
    up.z = 1.0f;
  } else {
    up.x = 1.0f;
    up.y = 0.0f;
    up.z = 0.0f;
  }
  const Vec3F vv = Normalize(Cross(nn, up));
  const Vec3F uu = Normalize(Cross(vv, nn));

  return Mat33F(uu.x, vv.x, nn.x,
    uu.y, vv.y, nn.y,
    uu.z, vv.z, nn.z);
}
/// @}

}  // namespace arctic

#endif  // ENGINE_MAT33F_H_
