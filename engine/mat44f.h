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

#ifndef ENGINE_MAT44F_H_
#define ENGINE_MAT44F_H_

#include <cmath>
#include "engine/arctic_pi.h"
#include "engine/arctic_types.h"
#include "engine/vec4f.h"

namespace arctic {

/// @addtogroup global_math
/// @{
struct Mat44F {
  float m[16];

  Mat44F() {}

  explicit Mat44F(float a00, float a01, float a02, float a03,
    float a04, float a05, float a06, float a07,
    float a08, float a09, float a10, float a11,
    float a12, float a13, float a14, float a15) {
    m[0] = a00;
    m[1] = a01;
    m[2] = a02;
    m[3] = a03;
    m[4] = a04;
    m[5] = a05;
    m[6] = a06;
    m[7] = a07;
    m[8] = a08;
    m[9] = a09;
    m[10] = a10;
    m[11] = a11;
    m[12] = a12;
    m[13] = a13;
    m[14] = a14;
    m[15] = a15;
  }

  explicit Mat44F(const float *v) {
    m[0] = v[0];
    m[1] = v[1];
    m[2] = v[2];
    m[3] = v[3];
    m[4] = v[4];
    m[5] = v[5];
    m[6] = v[6];
    m[7] = v[7];
    m[8] = v[8];
    m[9] = v[9];
    m[10] = v[10];
    m[11] = v[11];
    m[12] = v[12];
    m[13] = v[13];
    m[14] = v[14];
    m[15] = v[15];
  }

  float &operator[](Si32 i) {
    return m[i];
  }
  const float &operator[](Si32 i) const {
    return m[i];
  }
};

inline Mat44F operator*(Mat44F const &a, Mat44F const &b) {
  Mat44F res;
  for (Si32 i = 0; i < 4; i++) {
    const float x = a.m[4 * i + 0];
    const float y = a.m[4 * i + 1];
    const float z = a.m[4 * i + 2];
    const float w = a.m[4 * i + 3];

    res.m[4 * i + 0] = x * b[0] + y * b[4] + z * b[8] + w * b[12];
    res.m[4 * i + 1] = x * b[1] + y * b[5] + z * b[9] + w * b[13];
    res.m[4 * i + 2] = x * b[2] + y * b[6] + z * b[10] + w * b[14];
    res.m[4 * i + 3] = x * b[3] + y * b[7] + z * b[11] + w * b[15];
  }

  return res;
}

inline Vec4F operator*(Mat44F const &m, Vec4F const &v) {
  return Vec4F(v.x * m[0] + v.y * m[1] + v.z * m[2] + v.w * m[3],
    v.x * m[4] + v.y * m[5] + v.z * m[6] + v.w * m[7],
    v.x * m[8] + v.y * m[9] + v.z * m[10] + v.w * m[11],
    v.x * m[12] + v.y * m[13] + v.z * m[14] + v.w * m[15]);
}

inline Vec4F Transform(const Mat44F &m, const Vec4F &v) {
  return Vec4F(v.x * m[0] + v.y * m[1] + v.z * m[2] + v.w * m[3],
    v.x * m[4] + v.y * m[5] + v.z * m[6] + v.w * m[7],
    v.x * m[8] + v.y * m[9] + v.z * m[10] + v.w * m[11],
    v.x * m[12] + v.y * m[13] + v.z * m[14] + v.w * m[15]);
}

inline Vec3F Transform(const Mat44F &m, const Vec3F &v) {
  return Vec3F(v.x * m[0] + v.y * m[1] + v.z * m[2] + m[3],
    v.x * m[4] + v.y * m[5] + v.z * m[6] + m[7],
    v.x * m[8] + v.y * m[9] + v.z * m[10] + m[11]);
}

inline Vec3F Transform3(Mat44F const &m, Vec3F const &v) {
  return Vec3F(v.x * m[0] + v.y * m[1] + v.z * m[2],
    v.x * m[4] + v.y * m[5] + v.z * m[6],
    v.x * m[8] + v.y * m[9] + v.z * m[10]);
}

inline Mat44F Transpose(Mat44F const &m) {
  return Mat44F(m.m[0], m.m[4], m.m[8], m.m[12],
    m.m[1], m.m[5], m.m[9], m.m[13],
    m.m[2], m.m[6], m.m[10], m.m[14],
    m.m[3], m.m[7], m.m[11], m.m[15]);
}

inline Mat44F InvertFast(Mat44F const &m) {
  Mat44F inv = Mat44F(

    m.m[5] * m.m[10] * m.m[15] -
    m.m[5] * m.m[11] * m.m[14] -
    m.m[9] * m.m[6] * m.m[15] +
    m.m[9] * m.m[7] * m.m[14] +
    m.m[13] * m.m[6] * m.m[11] -
    m.m[13] * m.m[7] * m.m[10],

    -m.m[1] * m.m[10] * m.m[15] +
    m.m[1] * m.m[11] * m.m[14] +
    m.m[9] * m.m[2] * m.m[15] -
    m.m[9] * m.m[3] * m.m[14] -
    m.m[13] * m.m[2] * m.m[11] +
    m.m[13] * m.m[3] * m.m[10],

    m.m[1] * m.m[6] * m.m[15] -
    m.m[1] * m.m[7] * m.m[14] -
    m.m[5] * m.m[2] * m.m[15] +
    m.m[5] * m.m[3] * m.m[14] +
    m.m[13] * m.m[2] * m.m[7] -
    m.m[13] * m.m[3] * m.m[6],

    -m.m[1] * m.m[6] * m.m[11] +
    m.m[1] * m.m[7] * m.m[10] +
    m.m[5] * m.m[2] * m.m[11] -
    m.m[5] * m.m[3] * m.m[10] -
    m.m[9] * m.m[2] * m.m[7] +
    m.m[9] * m.m[3] * m.m[6],

    -m.m[4] * m.m[10] * m.m[15] +
    m.m[4] * m.m[11] * m.m[14] +
    m.m[8] * m.m[6] * m.m[15] -
    m.m[8] * m.m[7] * m.m[14] -
    m.m[12] * m.m[6] * m.m[11] +
    m.m[12] * m.m[7] * m.m[10],

    m.m[0] * m.m[10] * m.m[15] -
    m.m[0] * m.m[11] * m.m[14] -
    m.m[8] * m.m[2] * m.m[15] +
    m.m[8] * m.m[3] * m.m[14] +
    m.m[12] * m.m[2] * m.m[11] -
    m.m[12] * m.m[3] * m.m[10],

    -m.m[0] * m.m[6] * m.m[15] +
    m.m[0] * m.m[7] * m.m[14] +
    m.m[4] * m.m[2] * m.m[15] -
    m.m[4] * m.m[3] * m.m[14] -
    m.m[12] * m.m[2] * m.m[7] +
    m.m[12] * m.m[3] * m.m[6],


    m.m[0] * m.m[6] * m.m[11] -
    m.m[0] * m.m[7] * m.m[10] -
    m.m[4] * m.m[2] * m.m[11] +
    m.m[4] * m.m[3] * m.m[10] +
    m.m[8] * m.m[2] * m.m[7] -
    m.m[8] * m.m[3] * m.m[6],


    m.m[4] * m.m[9] * m.m[15] -
    m.m[4] * m.m[11] * m.m[13] -
    m.m[8] * m.m[5] * m.m[15] +
    m.m[8] * m.m[7] * m.m[13] +
    m.m[12] * m.m[5] * m.m[11] -
    m.m[12] * m.m[7] * m.m[9],



    -m.m[0] * m.m[9] * m.m[15] +
    m.m[0] * m.m[11] * m.m[13] +
    m.m[8] * m.m[1] * m.m[15] -
    m.m[8] * m.m[3] * m.m[13] -
    m.m[12] * m.m[1] * m.m[11] +
    m.m[12] * m.m[3] * m.m[9],

    m.m[0] * m.m[5] * m.m[15] -
    m.m[0] * m.m[7] * m.m[13] -
    m.m[4] * m.m[1] * m.m[15] +
    m.m[4] * m.m[3] * m.m[13] +
    m.m[12] * m.m[1] * m.m[7] -
    m.m[12] * m.m[3] * m.m[5],

    -m.m[0] * m.m[5] * m.m[11] +
    m.m[0] * m.m[7] * m.m[9] +
    m.m[4] * m.m[1] * m.m[11] -
    m.m[4] * m.m[3] * m.m[9] -
    m.m[8] * m.m[1] * m.m[7] +
    m.m[8] * m.m[3] * m.m[5],

    -m.m[4] * m.m[9] * m.m[14] +
    m.m[4] * m.m[10] * m.m[13] +
    m.m[8] * m.m[5] * m.m[14] -
    m.m[8] * m.m[6] * m.m[13] -
    m.m[12] * m.m[5] * m.m[10] +
    m.m[12] * m.m[6] * m.m[9],

    m.m[0] * m.m[9] * m.m[14] -
    m.m[0] * m.m[10] * m.m[13] -
    m.m[8] * m.m[1] * m.m[14] +
    m.m[8] * m.m[2] * m.m[13] +
    m.m[12] * m.m[1] * m.m[10] -
    m.m[12] * m.m[2] * m.m[9],

    -m.m[0] * m.m[5] * m.m[14] +
    m.m[0] * m.m[6] * m.m[13] +
    m.m[4] * m.m[1] * m.m[14] -
    m.m[4] * m.m[2] * m.m[13] -
    m.m[12] * m.m[1] * m.m[6] +
    m.m[12] * m.m[2] * m.m[5],

    m.m[0] * m.m[5] * m.m[10] -
    m.m[0] * m.m[6] * m.m[9] -
    m.m[4] * m.m[1] * m.m[10] +
    m.m[4] * m.m[2] * m.m[9] +
    m.m[8] * m.m[1] * m.m[6] -
    m.m[8] * m.m[2] * m.m[5]);

  float det = m.m[0] * inv.m[0]
    + m.m[1] * inv.m[4]
    + m.m[2] * inv.m[8]
    + m.m[3] * inv.m[12];
  det = 1.0f / det;
  for (Si32 i = 0; i < 16; i++) {
    inv.m[i] = inv.m[i] * det;
  }

  return inv;
}

inline Mat44F Invert(Mat44F const &src, Si32 *status = 0) {
  Si32   i, j, k, swap;
  float t, temp[4][4];


  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++) {
      temp[i][j] = src[i * 4 + j];
    }

  float inv[16];
  for (i = 0; i < 16; i++) {
    inv[i] = 0.0f;
  }
  inv[0] = 1.0f;
  inv[5] = 1.0f;
  inv[10] = 1.0f;
  inv[15] = 1.0f;

  for (i = 0; i < 4; i++) {
    // Look for largest element in column
    swap = i;
    for (j = i + 1; j < 4; j++)
      if (fabsf(temp[j][i]) > fabsf(temp[i][i])) {
        swap = j;
      }

    if (swap != i) {
      // Swap rows.
      for (k = 0; k < 4; k++) {
        t = temp[i][k];
        temp[i][k] = temp[swap][k];
        temp[swap][k] = t;

        t = inv[i * 4 + k];
        inv[i * 4 + k] = inv[swap * 4 + k];
        inv[swap * 4 + k] = t;
      }
    }

    // pivot==0 -> singular matrix!
    if (temp[i][i] == 0) {
      if (status) {
        status[0] = 0;
      }
      return Mat44F(0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f);
    }

    t = temp[i][i];
    t = 1.0f / t;
    for (k = 0; k < 4; k++) {
      temp[i][k] *= t;
      inv[i * 4 + k] *= t;
    }

    for (j = 0; j < 4; j++) {
      if (j != i) {
        t = temp[j][i];
        for (k = 0; k < 4; k++) {
          temp[j][k] -= temp[i][k] * t;
          inv[j * 4 + k] -= inv[i * 4 + k] * t;
        }
      }
    }
  }

  if (status) {
    status[0] = 1;
  }

  return Mat44F(inv[0], inv[1], inv[2], inv[3],
    inv[4], inv[5], inv[6], inv[7],
    inv[8], inv[9], inv[10], inv[11],
    inv[12], inv[13], inv[14], inv[15]);
}

inline Mat44F ExtractRotation(Mat44F const &m) {
  return Mat44F(m.m[0], m.m[4], m.m[8], 0.0f,
    m.m[1], m.m[5], m.m[9], 0.0f,
    m.m[2], m.m[6], m.m[10], 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Vec3F ExtractTranslation(Mat44F const &m) {
  return Vec3F(m.m[3], m.m[7], m.m[11]);
}

inline Vec3F ExtractScale(Mat44F const &m) {
  const Vec3F v1a = Vec3F(1.0f, 0.0f, 0.0f);
  const Vec3F v2a = Vec3F(0.0f, 1.0f, 0.0f);
  const Vec3F v3a = Vec3F(0.0f, 0.0f, 1.0f);
  const Vec3F v1b = (m * Vec4F(v1a, 0.0f)).xyz();
  const Vec3F v2b = (m * Vec4F(v2a, 0.0f)).xyz();
  const Vec3F v3b = (m * Vec4F(v3a, 0.0f)).xyz();

  return Vec3F(Length(v1b), Length(v2b), Length(v3b));
}

inline Vec3F ExtractRotationEuler(const Mat44F &m) {
  Vec3F res(0.0f);
  float cy = sqrtf(m.m[9] * m.m[9] + m.m[10] * m.m[10]);
  if (cy < 1e-6f) {
    res.y = (m.m[8] < 0.0f)
      ? static_cast<float>(kPi) / 2.0f
      : -static_cast<float>(kPi) / 2.0f;
    if (m.m[8] < 0.0f) {
      res.x = atan2f(m.m[1], m.m[2]);
    } else {
      res.x = atan2f(-m.m[1], -m.m[2]);
    }
    res.z = 0.0f;
  } else {
    res.x = atan2f(m.m[9], m.m[10]);
    res.y = atan2f(-m.m[8], cy);
    res.z = atan2f(m.m[4], m.m[0]);
  }
  return res;
}

inline Mat44F SetIdentity() {
  return Mat44F(1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}


inline Mat44F SetRotationQuaternion(const Vec4F &q) {
  float ww = q[3] * q[3];
  float xx = q[0] * q[0];
  float yy = q[1] * q[1];
  float zz = q[2] * q[2];

  return Mat44F(
    ww + xx - yy - zz,
    2.0f * (q[0] * q[1] - q[3] * q[2]),
    2.0f * (q[0] * q[2] + q[3] * q[1]),
    0.0f,

    2.0f * (q[0] * q[1] + q[3] * q[2]),
    ww - xx + yy - zz,
    2.0f * (q[1] * q[2] - q[3] * q[0]),
    0.0f,

    2.0f * (q[0] * q[2] - q[3] * q[1]),
    2.0f * (q[1] * q[2] + q[3] * q[0]),
    ww - xx - yy + zz,
    0.0f,

    0.0f,
    0.0f,
    0.0f,
    1.0f);
}

inline Mat44F SetRotationAxisAngle4(const Vec3F &a, const float t) {
  const float sint = sinf(t);
  const float cost = cosf(t);
  const float icost = 1.0f - cost;

  return Mat44F(a.x * a.x * icost + cost,
    a.y * a.x * icost - sint * a.z,
    a.z * a.x * icost + sint * a.y,
    0.0f,
    a.x * a.y * icost + sint * a.z,
    a.y * a.y * icost + cost,
    a.z * a.y * icost - sint * a.x,
    0.0f,
    a.x * a.z * icost - sint * a.y,
    a.y * a.z * icost + sint * a.x,
    a.z * a.z * icost + cost,
    0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetRotationX(const float t) {
  const float sint = sinf(t);
  const float cost = cosf(t);

  return Mat44F(1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, cost, -sint, 0.0f,
    0.0f, sint, cost, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetRotationY(const float t) {
  const float sint = sinf(t);
  const float cost = cosf(t);

  return Mat44F(cost, 0.0f, sint, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    -sint, 0.0f, cost, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetRotationZ(const float t) {
  const float sint = sinf(t);
  const float cost = cosf(t);

  return Mat44F(cost, -sint, 0.0f, 0.0f,
    sint, cost, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetRotationEuler4(const Vec3F &r) {
  const float a = sinf(r.x);
  const float b = cosf(r.x);
  const float c = sinf(r.y);
  const float d = cosf(r.y);
  const float e = sinf(r.z);
  const float f = cosf(r.z);

  return Mat44F(
    d * f, f * a * c - e * b, f * b * c + e * a, 0.0f,
    d * e, e * a * c + f * b, e * b * c - f * a, 0.0f,
    -c, d * a, d * b, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}


inline Mat44F SetTranslation(const Vec3F &p) {
  return Mat44F(1.0f, 0.0f, 0.0f, p.x,
    0.0f, 1.0f, 0.0f, p.y,
    0.0f, 0.0f, 1.0f, p.z,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetScale4(const Vec3F &s) {
  return Mat44F(s.x, 0.0f, 0.0f, 0.0f,
    0.0f, s.y, 0.0f, 0.0f,
    0.0f, 0.0f, s.z, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetIdentity4() {  //-V524
  return Mat44F(1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F SetRotationEuler4(float x, float y, float z) {
  return SetRotationEuler4(Vec3F(x, y, z));
}


inline Mat44F SetSwapYZ() {
  return Mat44F(1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}


inline Mat44F SetTranslation(float x, float y, float z) {
  return SetTranslation(Vec3F(x, y, z));
}

inline Mat44F SetScale4(float x, float y, float z) {
  return SetScale4(Vec3F(x, y, z));
}

inline Mat44F BuildBase4(const Vec3F &nn) {
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

  return Mat44F(uu.x, vv.x, nn.x, 0.0f,
    uu.y, vv.y, nn.y, 0.0f,
    uu.z, vv.z, nn.z, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline Mat44F BuildBase4(const Vec3F &u, const Vec3F &v, const Vec3F &n) {
  return Mat44F(u.x, v.x, n.x, 0.0f,
    u.y, v.y, n.y, 0.0f,
    u.z, v.z, n.z, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);
}

inline float Determinant(Mat44F const &m) {
  float inv0 = m[5] * m[10] * m[15] -
    m[5] * m[11] * m[14] -
    m[9] * m[6] * m[15] +
    m[9] * m[7] * m[14] +
    m[13] * m[6] * m[11] -
    m[13] * m[7] * m[10];

  float inv4 = -m[4] * m[10] * m[15] +
    m[4] * m[11] * m[14] +
    m[8] * m[6] * m[15] -
    m[8] * m[7] * m[14] -
    m[12] * m[6] * m[11] +
    m[12] * m[7] * m[10];

  float inv8 = m[4] * m[9] * m[15] -
    m[4] * m[11] * m[13] -
    m[8] * m[5] * m[15] +
    m[8] * m[7] * m[13] +
    m[12] * m[5] * m[11] -
    m[12] * m[7] * m[9];

  float inv12 = -m[4] * m[9] * m[14] +
    m[4] * m[10] * m[13] +
    m[8] * m[5] * m[14] -
    m[8] * m[6] * m[13] -
    m[12] * m[5] * m[10] +
    m[12] * m[6] * m[9];


  return m[0] * inv0 + m[1] * inv4 + m[2] * inv8 + m[3] * inv12;
}

inline Mat44F SetFrustumMat(float left, float right, float bottom, float top,
  float znear, float zfar) {
  const float x = (2.0f * znear) / (right - left);
  const float y = (2.0f * znear) / (top - bottom);
  const float a = (right + left) / (right - left);
  const float b = (top + bottom) / (top - bottom);
  const float c = -(zfar + znear) / (zfar - znear);
  const float d = -(2.0f * zfar * znear) / (zfar - znear);

  return Mat44F(x, 0.0f, a, 0.0f,
    0.0f, y, b, 0.0f,
    0.0f, 0.0f, c, d,
    0.0f, 0.0f, -1.0f, 0.0f);
  // inverse is:
  // return mat4x4( 1.0/x, 0.0f,  0.0f,   a/x,
  //                0.0f,  1.0/y, 0.0f,   b/x,
  //                0.0f,  0.0f,  0.0f,   -1.0,
  //                0.0f,  0.0f,  1.0f/d, c/d );
}

inline Vec2F GetNearFarFromPerspectiveMatrix(const Mat44F &m) {
  const float c = m.m[10];
  const float d = m.m[11];
  return Vec2F(d / (c - 1.0f), d / (c + 1.0f));
}
// Zeye = a / (Zbuffer[0..1] + b);
inline Vec2F GetZBufferToZetaEyeFromPerspectiveMatrix(const Mat44F &m) {
  return Vec2F(m.m[11] / 2.0f, (m.m[10] - 1.0f) / 2.0f);
}
// Zeye = a / (Zclip[-1..1] + b);
inline Vec2F GetZClipToZetaEyeFromPerspectiveMatrix(const Mat44F &m) {
  return Vec2F(0.0f, 0.0f);  // vec2(-m.m[11], m.m[10]);
}


inline Mat44F SetOrtho(float left, float right, float bottom, float top,
  float znear, float zfar) {
  Mat44F me;

  const float x = 2.0f / (right - left);
  const float y = 2.0f / (top - bottom);
  const float a = (right + left) / (right - left);
  const float b = (top + bottom) / (top - bottom);
  const float c = -2.0f / (zfar - znear);
  const float d = -(zfar + znear) / (zfar - znear);

  me[0] = x;
  me[1] = 0.0f;
  me[2] = 0.0f;
  me[3] = a;
  me[4] = 0.0f;
  me[5] = y;
  me[6] = 0.0f;
  me[7] = b;
  me[8] = 0.0f;
  me[9] = 0.0f;
  me[10] = c;
  me[11] = d;
  me[12] = 0.0f;
  me[13] = 0.0f;
  me[14] = 0.0f;
  me[15] = 1.0f;

  return me;
}

inline Mat44F SetPerspective(float fovy, float aspect,
  float znear, float zfar) {
#if 0
  const float ymax = znear * tanf(fovy * 3.141592653589f / 360.0f);
  const float ymin = -ymax;
  const float xmin = ymin * aspect;
  const float xmax = ymax * aspect;

  return setFrustumMat(xmin, xmax, ymin, ymax, znear, zfar);
#else
  const float tan = tanf(fovy * 3.141592653589f / 360.0f);
  const float x = 1.0f / (tan * aspect);
  const float y = 1.0f / (tan);
  const float c = -(zfar + znear) / (zfar - znear);
  const float d = -(2.0f * zfar * znear) / (zfar - znear);

  return Mat44F(x, 0.0f, 0.0f, 0.0f,
    0.0f, y, 0.0f, 0.0f,
    0.0f, 0.0f, c, d,
    0.0f, 0.0f, -1.0f, 0.0f);
  // inverse is:
  // return mat4x4( tan*aspect, 0.0f,  0.0f,   0.0f,
  //                0.0f,       tan,   0.0f,   0.0f,
  //                0.0f,       0.0f,  0.0f,  -1.0f,
  //                0.0f,       0.0f,  -(zfar-znear)/(2.0f*zfar*znear),
  //                                          (zfar+znear)/(2.0f*zfar*znear) );
#endif  // 0
}

inline Mat44F SetProjection(const Vec4F &fov, float znear, float zfar) {
#if 0
  const float ymax = znear * fov.x;
  const float ymin = -znear * fov.y;
  const float xmin = -znear * fov.z;
  const float xmax = znear * fov.w;

  return setFrustumMat(xmin, xmax, ymin, ymax, znear, zfar);
#else
  float x = 2.0f / (fov.w + fov.z);
  float y = 2.0f / (fov.x + fov.y);
  float a = (fov.w - fov.z) / (fov.w + fov.z);
  float b = (fov.x - fov.y) / (fov.x + fov.y);
  float c = -(zfar + znear) / (zfar - znear);
  float d = -(2.0f * zfar * znear) / (zfar - znear);
  return Mat44F(x, 0.0f, a, 0.0f,
    0.0f, y, b, 0.0f,
    0.0f, 0.0f, c, d,
    0.0f, 0.0f, -1.0f, 0.0f);
  // inverse is:
  // return mat4x4( 1.0/x, 0.0f,  0.0f,   a/x,
  //               0.0f,  1.0/y, 0.0f,   b/x,
  //               0.0f,  0.0f,  0.0f,   -1.0,
  //               0.0f,  0.0f,  1.0f/d, c/d );
#endif  // 0
}

inline Mat44F SetPerspectiveTiled(float fovy, float aspect,
  float znear, float zfar, const Vec2F &off, const Vec2F &wid) {
  float ym = znear * tanf(fovy * 3.141592653589f / 360.0f);
  float xm = ym * aspect;

  const float xmin = -xm + 2.0f * xm * off.x;
  const float ymin = -ym + 2.0f * ym * off.y;
  const float xmax = xmin + 2.0f * xm * wid.x;
  const float ymax = ymin + 2.0f * ym * wid.y;

  return SetFrustumMat(xmin, xmax, ymin, ymax, znear, zfar);
}

//
// faceid:
//
//        -----
//        | 1 |
//   ------------------
//   | 0  | 2 | 4 | 5 |
//   ------------------
//        | 3 |
//        -----
//
inline Mat44F SetCubeFaceTiled(Si32 faceid,
  float znear, float zfar, const Vec2F &off, const Vec2F &wid) {
  const float w = znear;

  float xmin = -w + 2.0f * w * off.x;
  float ymin = -w + 2.0f * w * off.y;
  float xmax = xmin + 2.0f * w * wid.x;
  float ymax = ymin + 2.0f * w * wid.y;

  Mat44F m = SetFrustumMat(xmin, xmax, ymin, ymax, znear, zfar);

  if (faceid == 0) {
    m = m * Mat44F(
      0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  }
  if (faceid == 1) {
    m = m * Mat44F(
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  }
  if (faceid == 3) {
    m = m * Mat44F(
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, -1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  }
  if (faceid == 4) {
    m = m * Mat44F(
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      -1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  }
  if (faceid == 5) {
    m = m * Mat44F(
      -1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  }
  return m;
}


inline Mat44F SetLookat(const Vec3F &eye, const Vec3F &tar, const Vec3F &up) {
  const float dir[3] = {
    -tar[0] + eye[0],
    -tar[1] + eye[1],
    -tar[2] + eye[2]
  };

  const float kEps = 1e-12f;

  float dir_len2 = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
  if (dir_len2 < kEps) {
    return SetIdentity();
  }

  Mat44F mat;

  // right vector = up x dir
  mat[0] = dir[2] * up[1] - dir[1] * up[2];
  mat[1] = dir[0] * up[2] - dir[2] * up[0];
  mat[2] = dir[1] * up[0] - dir[0] * up[1];
  float right_len2 = mat[0] * mat[0] + mat[1] * mat[1] + mat[2] * mat[2];
  if (right_len2 < kEps) {
    return SetIdentity();
  }
  float im = 1.0f / sqrtf(right_len2);
  mat[0] *= im;
  mat[1] *= im;
  mat[2] *= im;

  // up vector = dir x right
  mat[4] = mat[2] * dir[1] - mat[1] * dir[2];
  mat[5] = mat[0] * dir[2] - mat[2] * dir[0];
  mat[6] = mat[1] * dir[0] - mat[0] * dir[1];
  float up_len2 = mat[4] * mat[4] + mat[5] * mat[5] + mat[6] * mat[6];
  if (up_len2 < kEps) {
    return SetIdentity();
  }
  im = 1.0f / sqrtf(up_len2);
  mat[4] *= im;
  mat[5] *= im;
  mat[6] *= im;

  // view vector
  mat[8] = dir[0];
  mat[9] = dir[1];
  mat[10] = dir[2];
  im = 1.0f / sqrtf(dir_len2);
  mat[8] *= im;
  mat[9] *= im;
  mat[10] *= im;

  mat[3] = -(mat[0] * eye[0] + mat[1] * eye[1] + mat[2] * eye[2]);
  mat[7] = -(mat[4] * eye[0] + mat[5] * eye[1] + mat[6] * eye[2]);
  mat[11] = -(mat[8] * eye[0] + mat[9] * eye[1] + mat[10] * eye[2]);

  mat[12] = 0.0f;
  mat[13] = 0.0f;
  mat[14] = 0.0f;
  mat[15] = 1.0f;

  return mat;
}
/// @}

}  // namespace arctic

#endif  // ENGINE_MAT44F_H_
