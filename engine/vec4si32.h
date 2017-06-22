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

  explicit Vec4Si32(struct Vec4F &s);

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
