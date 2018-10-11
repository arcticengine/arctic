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

#ifndef ENGINE_VEC4F_H_
#define ENGINE_VEC4F_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {
struct Vec4F {
  union {
    struct {
      float x;
      float y;
      float z;
      float w;
    };
    float element[4];
  };

  Vec4F() {}

  explicit Vec4F(float a, float b, float c, float d) {
    x = a;
    y = b;
    z = c;
    w = d;
  }

  explicit Vec4F(float const *const v) {
    x = v[0];
    y = v[1];
    z = v[2];
    w = v[3];
  }

  explicit Vec4F(Vec3F const &v, const float s) {
    x = v.x;
    y = v.y;
    z = v.z;
    w = s;
  }

  explicit Vec4F(Vec2F const &a, Vec2F const &b) {
    x = a.x;
    y = a.y;
    z = b.x;
    w = b.y;
  }

  explicit Vec4F(const float s, Vec3F const &v) {
    x = s;
    y = v.x;
    z = v.y;
    w = v.z;
  }

  explicit Vec4F(float s) {
    x = s;
    y = s;
    z = s;
    w = s;
  }

  float &operator[](Si32 i) {
    return element[i];
  }
  const float &operator[](Si32 i) const {
    return element[i];
  }

  Vec4F &operator =(Vec4F const &v) {
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
    return *this;
  }
  Vec4F &operator+=(float const &s) {
    x += s;
    y += s;
    z += s;
    w += s;
    return *this;
  }
  Vec4F &operator+=(Vec4F const &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
  }
  Vec4F &operator-=(float const &s) {
    x -= s;
    y -= s;
    z -= s;
    w -= s;
    return *this;
  }
  Vec4F &operator-=(Vec4F const &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
  }
  Vec4F &operator*=(float const &s) {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
  }
  Vec4F &operator*=(Vec4F const &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    w *= v.w;
    return *this;
  }
  Vec4F &operator/=(float const &s) {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
    return *this;
  }
  Vec4F &operator/=(Vec4F const &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    w /= v.w;
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
  Vec2F wz() const {
    return Vec2F(w, z);
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
  Vec3F xxw() const {
    return Vec3F(x, x, w);
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
  Vec3F xyw() const {
    return Vec3F(x, y, w);
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
  Vec3F xzw() const {
    return Vec3F(x, z, w);
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
  Vec3F yxw() const {
    return Vec3F(y, x, w);
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
  Vec3F yyw() const {
    return Vec3F(y, y, w);
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
  Vec3F yzw() const {
    return Vec3F(y, z, w);
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
  Vec3F zxw() const {
    return Vec3F(z, x, w);
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
  Vec3F zzw() const {
    return Vec3F(z, z, w);
  }
  Vec3F www() const {
    return Vec3F(w, w, w);
  }
};

inline Vec4F operator+(Vec4F const &v, float const &s) {
  return Vec4F(v.x + s, v.y + s, v.z + s, v.w + s);
}
inline Vec4F operator+(float const &s, Vec4F const &v) {
  return Vec4F(s + v.x, s + v.y, s + v.z, s + v.w);
}
inline Vec4F operator+(Vec4F const &a, Vec4F const &b) {
  return Vec4F(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
inline Vec4F operator-(Vec4F const &v, float const &s) {
  return Vec4F(v.x - s, v.y - s, v.z - s, v.w - s);
}
inline Vec4F operator-(float const &s, Vec4F const &v) {
  return Vec4F(s - v.x, s - v.y, s - v.z, s - v.w);
}
inline Vec4F operator-(Vec4F const &a, Vec4F const &b) {
  return Vec4F(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
inline Vec4F operator*(Vec4F const &v, float const &s) {
  return Vec4F(v.x * s, v.y * s, v.z * s, v.w * s);
}
inline Vec4F operator*(float const &s, Vec4F const &v) {
  return Vec4F(s * v.x, s * v.y, s * v.z, s * v.w);
}
inline Vec4F operator*(Vec4F const &a, Vec4F const &b) {
  return Vec4F(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
inline Vec4F operator/(Vec4F const &v, float const &s) {
  return Vec4F(v.x / s, v.y / s, v.z / s, v.w / s);
}
inline Vec4F operator/(float const &s, Vec4F const &v) {
  return Vec4F(s / v.x, s / v.y, s / v.z, s / v.w);
}
inline Vec4F operator/(Vec4F const &a, Vec4F const &b) {
  return Vec4F(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

inline Vec4F Normalize(Vec4F const &v) {
  float const m2 = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
  float const im = 1.0f / sqrtf(m2);
  return Vec4F(v.x * im, v.y * im, v.z * im, v.w * im);
}

inline Vec4F Mix(Vec4F const &a, Vec4F const &b, float const f) {
  return Vec4F(a.x * (1.0f - f) + f * b.x,
    a.y * (1.0f - f) + f * b.y,
    a.z * (1.0f - f) + f * b.z,
    a.w * (1.0f - f) + f * b.w);
}

inline float Length(Vec4F const &v) {
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline float Dot(Vec4F const &a, Vec4F const &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float Distance(Vec4F const &a, Vec4F const &b) {
  return Length(a - b);
}

// make a plane that goes through pointa a, b, c
inline Vec4F MakePlane(Vec3F const &a, Vec3F const &b, Vec3F const &c) {
  //    const vec3 n = normalize( cross( c-a, b-a ) );
  const Vec3F n = Normalize(Cross(b - a, c - a));
  return Vec4F(n, -Dot(a, n));
}

inline Vec3F IntersectPlanes(Vec4F const &p1, Vec4F const &p2,
    Vec4F const &p3) {
  const float den = Dot(p1.xyz(), Cross(p2.xyz(), p3.xyz()));

  if (den == 0.0f) {
    return Vec3F(0.0f);
  }

  Vec3F res = p1.w * Cross(p2.xyz(), p3.xyz()) +
    p2.w * Cross(p3.xyz(), p1.xyz()) +
    p3.w * Cross(p1.xyz(), p2.xyz());

  return res * (-1.0f / den);
}

}  // namespace arctic

#endif  // ENGINE_VEC4F_H_
