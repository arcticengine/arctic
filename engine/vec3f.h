// The MIT License(MIT)
//
// Copyright 2015 - 2016 Inigo Quilez
// Copyright 2016 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#ifndef ENGINE_VEC3F_H_
#define ENGINE_VEC3F_H_

#include <cmath>
#include "engine/arctic_types.h"
#include "engine/vec2f.h"
#include "engine/vec3si32.h"

namespace arctic {

struct Vec3F {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float element[3];
  };

  Vec3F() {}

  explicit Vec3F(float a) {
    x = a;
    y = a;
    z = a;
  }

  explicit Vec3F(float a, float b, float c) {
    x = a;
    y = b;
    z = c;
  }

  explicit Vec3F(const float *v) {
    x = v[0];
    y = v[1];
    z = v[2];
  }

  explicit Vec3F(Vec2F const &v, float s) {
    x = v.x;
    y = v.y;
    z = s;
  }

  explicit Vec3F(float s, Vec2F const &v) {
    x = s;
    y = v.x;
    z = v.y;
  }

  explicit Vec3F(const Vec3Si32 &v) {
    x = static_cast<float>(v.x);
    y = static_cast<float>(v.y);
    z = static_cast<float>(v.z);
  }

  float &operator[](Si32 i) {
    return element[i];
  }
  const float &operator[](Si32 i) const {
    return element[i];
  }

  Vec3F &operator =(Vec3F  const &v) {
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
  }
  Vec3F &operator+=(float const &s) {
    x += s;
    y += s;
    z += s;
    return *this;
  }
  Vec3F &operator+=(Vec3F  const &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  Vec3F &operator-=(float const &s) {
    x -= s;
    y -= s;
    z -= s;
    return *this;
  }
  Vec3F &operator-=(Vec3F  const &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }
  Vec3F &operator*=(float const &s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }
  Vec3F &operator*=(Vec3F  const &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
  }
  Vec3F &operator/=(float const &s) {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }
  Vec3F &operator/=(Vec3F  const &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    return *this;
  }

  Vec2F xx() const {
    return Vec2F(x, x);
  }
  Vec2F xy() const {
    return Vec2F(x, y);
  }
  Vec2F xz() const {
    return Vec2F(x, z);
  }
  Vec2F yx() const {
    return Vec2F(y, x);
  }
  Vec2F yy() const {
    return Vec2F(y, y);
  }
  Vec2F yz() const {
    return Vec2F(y, z);
  }
  Vec2F zx() const {
    return Vec2F(z, x);
  }
  Vec2F zy() const {
    return Vec2F(z, y);
  }
  Vec2F zz() const {
    return Vec2F(z, z);
  }

  Vec3F xxx() const {
    return Vec3F(x, x, x);
  }
  Vec3F xxy() const {
    return Vec3F(x, x, y);
  }
  Vec3F xxz() const {
    return Vec3F(x, x, z);
  }
  Vec3F xyx() const {
    return Vec3F(x, y, x);
  }
  Vec3F xyy() const {
    return Vec3F(x, y, y);
  }
  Vec3F xyz() const {
    return Vec3F(x, y, z);
  }
  Vec3F xzx() const {
    return Vec3F(x, z, x);
  }
  Vec3F xzy() const {
    return Vec3F(x, z, y);
  }
  Vec3F xzz() const {
    return Vec3F(x, z, z);
  }
  Vec3F yxx() const {
    return Vec3F(y, x, x);
  }
  Vec3F yxy() const {
    return Vec3F(y, x, y);
  }
  Vec3F yxz() const {
    return Vec3F(y, x, z);
  }
  Vec3F yyx() const {
    return Vec3F(y, y, x);
  }
  Vec3F yyy() const {
    return Vec3F(y, y, y);
  }
  Vec3F yyz() const {
    return Vec3F(y, y, z);
  }
  Vec3F yzx() const {
    return Vec3F(y, z, x);
  }
  Vec3F yzy() const {
    return Vec3F(y, z, y);
  }
  Vec3F yzz() const {
    return Vec3F(y, z, z);
  }
  Vec3F zxx() const {
    return Vec3F(z, x, x);
  }
  Vec3F zxy() const {
    return Vec3F(z, x, y);
  }
  Vec3F zxz() const {
    return Vec3F(z, x, z);
  }
  Vec3F zyx() const {
    return Vec3F(z, y, x);
  }
  Vec3F zyy() const {
    return Vec3F(z, y, y);
  }
  Vec3F zyz() const {
    return Vec3F(z, y, z);
  }
  Vec3F zzx() const {
    return Vec3F(z, z, x);
  }
  Vec3F zzy() const {
    return Vec3F(z, z, y);
  }
  Vec3F zzz() const {
    return Vec3F(z, z, z);
  }
};

inline Vec3F operator+(const Vec3F &v, float const &s) {
  return Vec3F(v.x + s, v.y + s, v.z + s);
}
inline Vec3F operator+(float const &s, Vec3F  const &v) {
  return Vec3F(s + v.x, s + v.y, s + v.z);
}
inline Vec3F operator+(Vec3F  const &a, Vec3F  const &b) {
  return Vec3F(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline Vec3F operator-(Vec3F  const &v, float const &s) {
  return Vec3F(v.x - s, v.y - s, v.z - s);
}
inline Vec3F operator-(float const &s, Vec3F  const &v) {
  return Vec3F(s - v.x, s - v.y, s - v.z);
}
inline Vec3F operator-(Vec3F  const &a, Vec3F  const &b) {
  return Vec3F(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline Vec3F operator*(Vec3F  const &v, float const &s) {
  return Vec3F(v.x * s, v.y * s, v.z * s);
}
inline Vec3F operator*(float const &s, Vec3F  const &v) {
  return Vec3F(s * v.x, s * v.y, s * v.z);
}
inline Vec3F operator*(Vec3F  const &a, Vec3F  const &b) {
  return Vec3F(a.x * b.x, a.y * b.y, a.z * b.z);
}
inline Vec3F operator/(Vec3F  const &v, float const &s) {
  return Vec3F(v.x / s, v.y / s, v.z / s);
}
inline Vec3F operator/(float const &s, Vec3F  const &v) {
  return Vec3F(s / v.x, s / v.y, s / v.z);
}
inline Vec3F operator/(Vec3F  const &a, Vec3F  const &b) {
  return Vec3F(a.x / b.x, a.y / b.y, a.z / b.z);
}

inline Vec3F floor(Vec3F const &v) {
  return Vec3F(floorf(v.x), floorf(v.y), floorf(v.z));
}

inline Vec3F Normalize(Vec3F const &v) {
  float const m2 = v.x * v.x + v.y * v.y + v.z * v.z;
  float const im = 1.0f / sqrtf(m2);
  return Vec3F(v.x * im, v.y * im, v.z * im);
}

inline Vec3F NormalizeSafe(Vec3F const &v) {
  float const m2 = v.x * v.x + v.y * v.y + v.z * v.z;
  if (m2 <= 0.000000001f) {
    return Vec3F(0.0f);
  }
  float const im = 1.0f / sqrtf(m2);
  return Vec3F(v.x * im, v.y * im, v.z * im);
}

inline Vec3F Mix(Vec3F const &a, Vec3F const &b, float const f) {
  return Vec3F(a.x * (1.0f - f) + f * b.x,
    a.y * (1.0f - f) + f * b.y,
    a.z * (1.0f - f) + f * b.z);
}

inline Vec3F Cross(Vec3F const &a, Vec3F const &b) {
  return Vec3F(a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x);
}

inline float Length(Vec3F const &v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float LengthSquared(Vec3F const &v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float InverseLength(Vec3F const &v) {
  return 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float Dot(Vec3F const &a, Vec3F const &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float Distance(Vec3F const &a, Vec3F const &b) {
  return Length(a - b);
}

inline void BuildBase(const Vec3F &n, Vec3F *uu, Vec3F *vv) {
  Vec3F up;
  if (fabsf(n.z) < 0.9f) {
    up.x = 0.0f;
    up.y = 0.0f;
    up.z = 1.0f;
  } else {
    up.x = 1.0f;
    up.y = 0.0f;
    up.z = 0.0f;
  }
  *vv = Normalize(Cross(n, up));
  *uu = Normalize(Cross(*vv, n));
}


inline Vec3F Orientate(const Vec3F &v, const Vec3F &dir) {
  Vec3F res = v;
  const float kk = Dot(dir, v);
  if (kk < 0.0f) {
    res -= 2.0f * dir * kk;
  }
  return res;
}

inline Vec3F pow(const Vec3F &v, const float f) {
  return Vec3F(powf(v.x, f), powf(v.y, f), powf(v.z, f));
}

inline Vec3F pow(const Vec3F &v, const Vec3F &f) {
  return Vec3F(powf(v.x, f.x), powf(v.y, f.y), powf(v.z, f.z));
}

inline Vec3F sin(const Vec3F &v) {
  return Vec3F(sinf(v.x), sinf(v.y), sinf(v.z));
}

inline Vec3F cos(const Vec3F &v) {
  return Vec3F(cosf(v.x), cosf(v.y), cosf(v.z));
}

inline Vec3F mod(const Vec3F &v, float s) {
  return Vec3F(fmodf(v.x, s), fmodf(v.y, s), fmodf(v.z, s));
}

inline Vec3F sqrt(const Vec3F &v) {
  return Vec3F(sqrtf(v.x), sqrtf(v.y), sqrtf(v.z));
}

inline Vec3F Min(const Vec3F &v, float mi) {
  return Vec3F((v.x < mi) ? v.x : mi,
    (v.y < mi) ? v.y : mi,
    (v.z < mi) ? v.z : mi);
}
inline Vec3F Max(const Vec3F &v, float ma) {
  return Vec3F((v.x > ma) ? v.x : ma,
    (v.y > ma) ? v.y : ma,
    (v.z > ma) ? v.z : ma);
}
inline Vec3F Clamp(const Vec3F &v, float mi, float ma) {
  return Max(Min(v, ma), mi);
}
inline Vec3F Clamp01(const Vec3F &v) {
  return Max(Min(v, 1.0f), 0.0f);
}
inline Vec3F Clamp1(const Vec3F &v) {
  return Max(Min(v, 1.0f), -1.0f);  // NOLINT
}
inline Vec3F Abs(const Vec3F &v) {
  return Vec3F(fabsf(v.x), fabsf(v.y), fabsf(v.z));
}

inline Vec3F SmoothStep(float a, float b, const Vec3F &v) {
  Vec3F x = Clamp01((v - Vec3F(a)) / (b - a));
  return x * x * (3.0f - 2.0f * x);
}

inline float MaxComp(const Vec3F &v) {
  return (v.x > v.y) ? ((v.x > v.z) ? v.x : v.z) : ((v.y > v.z) ? v.y : v.z);
}


}  // namespace arctic

#endif  // ENGINE_VEC3F_H_
