// The MIT License(MIT)
//
// Copyright 2015 - 2016 Inigo Quilez
// Copyright 2016 - 2017 Huldra
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

#ifndef ENGINE_VEC4SI32_H_
#define ENGINE_VEC4SI32_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

struct Vec4Si32 {
  union {
    struct {
      Si32 x;
      Si32 y;
      Si32 z;
      Si32 w;
    };
    Si32 element[4];
  };

  Vec4Si32() {}

  explicit Vec4Si32(const struct Vec4F &s);

  explicit Vec4Si32(Si32 a, Si32 b, Si32 c, Si32 d) {
    x = a;
    y = b;
    z = c;
    w = d;
  }
  explicit Vec4Si32(Si32 s) {
    x = s;
    y = s;
    z = s;
    w = s;
  }

  Si32 &operator[](Si32 i) {
    return element[i];
  }
  const Si32 &operator[](Si32 i) const {
    return element[i];
  }

  Vec4Si32 &operator =(const Vec4Si32 &v) {
      x = v.x;
      y = v.y;
      z = v.z;
      w = v.w;
      return *this;
  }
  Vec4Si32 &operator+=(const Si32 &s) {
      x += s;
      y += s;
      z += s;
      w += s;
      return *this;
  }
  Vec4Si32 &operator+=(const Vec4Si32 &v) {
      x += v.x;
      y += v.y;
      z += v.z;
      w += v.w;
      return *this;
  }
  Vec4Si32 &operator-=(const Si32 &s) {
      x -= s;
      y -= s;
      z -= s;
      w -= s;
      return *this;
  }
  Vec4Si32 &operator-=(const Vec4Si32 &v) {
      x -= v.x;
      y -= v.y;
      z -= v.z;
      w -= v.w;
      return *this;
  }
  Vec4Si32 &operator*=(const Si32 &s) {
      x *= s;
      y *= s;
      z *= s;
      w *= s;
      return *this;
  }
  Vec4Si32 &operator*=(const Vec4Si32 &v) {
      x *= v.x;
      y *= v.y;
      z *= v.z;
      w *= v.w;
      return *this;
  }
  Vec4Si32 &operator/=(const Si32 &s) {
      x /= s;
      y /= s;
      z /= s;
      w /= s;
      return *this;
  }
  Vec4Si32 &operator/=(const Vec4Si32 &v) {
      x /= v.x;
      y /= v.y;
      z /= v.z;
      w /= v.w;
      return *this;
  }

  const bool operator== (const Vec4Si32 &v) const {
      return x == v.x && y == v.y && z == v.z && w == v.w;
  }
  const bool operator!= (const Vec4Si32 &v) const {
      return x != v.x || y != v.y || z != v.z || w != v.w;
  }
  
  Vec2Si32 xx() const {
    return Vec2Si32(x, x);
  }
  Vec2Si32 xy() const {
    return Vec2Si32(x, y);
  }
  Vec2Si32 xz() const {
    return Vec2Si32(x, z);
  }
  Vec2Si32 yx() const {
    return Vec2Si32(y, x);
  }
  Vec2Si32 yy() const {
    return Vec2Si32(y, y);
  }
  Vec2Si32 yz() const {
    return Vec2Si32(y, z);
  }
  Vec2Si32 zx() const {
    return Vec2Si32(z, x);
  }
  Vec2Si32 zy() const {
    return Vec2Si32(z, y);
  }
  Vec2Si32 zz() const {
    return Vec2Si32(z, z);
  }
  Vec2Si32 wz() const {
    return Vec2Si32(w, z);
  }
  
  Vec3Si32 xxx() const {
    return Vec3Si32(x, x, x);
  }
  Vec3Si32 xxy() const {
    return Vec3Si32(x, x, y);
  }
  Vec3Si32 xxz() const {
    return Vec3Si32(x, x, z);
  }
  Vec3Si32 xxw() const {
    return Vec3Si32(x, x, w);
  }
  Vec3Si32 xyx() const {
    return Vec3Si32(x, y, x);
  }
  Vec3Si32 xyy() const {
    return Vec3Si32(x, y, y);
  }
  Vec3Si32 xyz() const {
    return Vec3Si32(x, y, z);
  }
  Vec3Si32 xyw() const {
    return Vec3Si32(x, y, w);
  }
  Vec3Si32 xzx() const {
    return Vec3Si32(x, z, x);
  }
  Vec3Si32 xzy() const {
    return Vec3Si32(x, z, y);
  }
  Vec3Si32 xzz() const {
    return Vec3Si32(x, z, z);
  }
  Vec3Si32 xzw() const {
    return Vec3Si32(x, z, w);
  }
  Vec3Si32 yxx() const {
    return Vec3Si32(y, x, x);
  }
  Vec3Si32 yxy() const {
    return Vec3Si32(y, x, y);
  }
  Vec3Si32 yxz() const {
    return Vec3Si32(y, x, z);
  }
  Vec3Si32 yxw() const {
    return Vec3Si32(y, x, w);
  }
  Vec3Si32 yyx() const {
    return Vec3Si32(y, y, x);
  }
  Vec3Si32 yyy() const {
    return Vec3Si32(y, y, y);
  }
  Vec3Si32 yyz() const {
    return Vec3Si32(y, y, z);
  }
  Vec3Si32 yyw() const {
    return Vec3Si32(y, y, w);
  }
  Vec3Si32 yzx() const {
    return Vec3Si32(y, z, x);
  }
  Vec3Si32 yzy() const {
    return Vec3Si32(y, z, y);
  }
  Vec3Si32 yzz() const {
    return Vec3Si32(y, z, z);
  }
  Vec3Si32 yzw() const {
    return Vec3Si32(y, z, w);
  }
  Vec3Si32 zxx() const {
    return Vec3Si32(z, x, x);
  }
  Vec3Si32 zxy() const {
    return Vec3Si32(z, x, y);
  }
  Vec3Si32 zxz() const {
    return Vec3Si32(z, x, z);
  }
  Vec3Si32 zxw() const {
    return Vec3Si32(z, x, w);
  }
  Vec3Si32 zyx() const {
    return Vec3Si32(z, y, x);
  }
  Vec3Si32 zyy() const {
    return Vec3Si32(z, y, y);
  }
  Vec3Si32 zyz() const {
    return Vec3Si32(z, y, z);
  }
  Vec3Si32 zzx() const {
    return Vec3Si32(z, z, x);
  }
  Vec3Si32 zzy() const {
    return Vec3Si32(z, z, y);
  }
  Vec3Si32 zzz() const {
    return Vec3Si32(z, z, z);
  }
  Vec3Si32 zzw() const {
    return Vec3Si32(z, z, w);
  }
  Vec3Si32 www() const {
    return Vec3Si32(w, w, w);
  }
};

inline Vec4Si32 operator+(Vec4Si32 const &v, Si32 const &s) {
    return Vec4Si32(v.x + s, v.y + s, v.z + s, v.w + s);
}
inline Vec4Si32 operator+(Si32 const &s, Vec4Si32 const &v) {
    return Vec4Si32(s + v.x, s + v.y, s + v.z, s + v.w);
}
inline Vec4Si32 operator+(Vec4Si32 const &a, Vec4Si32 const &b) {
    return Vec4Si32(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
inline Vec4Si32 operator-(Vec4Si32 const &v, Si32 const &s) {
    return Vec4Si32(v.x - s, v.y - s, v.z - s, v.w - s);
}
inline Vec4Si32 operator-(Si32 const &s, Vec4Si32 const &v) {
    return Vec4Si32(s - v.x, s - v.y, s - v.z, s - v.w);
}
inline Vec4Si32 operator-(Vec4Si32 const &a, Vec4Si32 const &b) {
    return Vec4Si32(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
inline Vec4Si32 operator*(Vec4Si32 const &v, Si32 const &s) {
    return Vec4Si32(v.x * s, v.y * s, v.z * s, v.w * s);
}
inline Vec4Si32 operator*(Si32 const &s, Vec4Si32 const &v) {
    return Vec4Si32(s * v.x, s * v.y, s * v.z, s * v.w);
}
inline Vec4Si32 operator*(Vec4Si32 const &a, Vec4Si32 const &b) {
    return Vec4Si32(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
inline Vec4Si32 operator/(Vec4Si32 const &v, Si32 const &s) {
    return Vec4Si32(v.x / s, v.y / s, v.z / s, v.w / s);
}
inline Vec4Si32 operator/(Si32 const &s, Vec4Si32 const &v) {
    return Vec4Si32(s / v.x, s / v.y, s / v.z, s / v.w);
}
inline Vec4Si32 operator/(Vec4Si32 const &a, Vec4Si32 const &b) {
    return Vec4Si32(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

inline Vec4Si32 Min(const Vec4Si32 &v, Si32 mi) {
    return Vec4Si32((v.x > mi) ? mi : v.x,
        (v.y > mi) ? mi : v.y,
        (v.z > mi) ? mi : v.z,
        (v.w > mi) ? mi : v.w);
}

inline Vec4Si32 Max(const Vec4Si32 &v, Si32 ma) {  // NOLINT
    return Vec4Si32((v.x < ma) ? ma : v.x,
        (v.y < ma) ? ma : v.y,
        (v.z < ma) ? ma : v.z,
        (v.w < ma) ? ma : v.w);
}

inline Vec4Si32 Clamp(const Vec4Si32 &v, Si32 mi, Si32 ma) {
    return Vec4Si32((v.x < mi) ? mi : ((v.x > ma) ? ma : v.x),
        (v.y < mi) ? mi : ((v.y > ma) ? ma : v.y),
        (v.z < mi) ? mi : ((v.z > ma) ? ma : v.z),
        (v.w < mi) ? mi : ((v.w > ma) ? ma : v.w));
}

}  // namespace arctic

#endif  // ENGINE_VEC4SI32_H_
