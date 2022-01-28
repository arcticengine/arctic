// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 - 2021 Huldra
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

#ifndef ENGINE_VEC2SI32_H_
#define ENGINE_VEC2SI32_H_

#define _USE_MATH_DEFINES
#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_math
/// @{
struct Vec2Si32 {
  union {
    struct {
      Si32 x;
      Si32 y;
    };
    Si32 element[2];
  };

  Vec2Si32() {}

  explicit Vec2Si32(const struct Vec2F &s);
  explicit Vec2Si32(const struct Vec2D &s);

  explicit Vec2Si32(Si32 a, Si32 b) noexcept {
    x = a;
    y = b;
  }

  Si32 &operator[](Si32 i) {
    return element[i];
  }
  const Si32 &operator[](Si32 i) const {
    return element[i];
  }

  Vec2Si32 &operator =(Vec2Si32  const &v) {
    x = v.x;
    y = v.y;
    return *this;
  }
  Vec2Si32 &operator+=(Si32 const &s) {
    x += s;
    y += s;
    return *this;
  }
  Vec2Si32 &operator+=(Vec2Si32  const &v) {
    x += v.x;
    y += v.y;
    return *this;
  }
  Vec2Si32 &operator-=(Si32 const &s) {
    x -= s;
    y -= s;
    return *this;
  }
  Vec2Si32 &operator-=(Vec2Si32  const &v) {
    x -= v.x;
    y -= v.y;
    return *this;
  }
  Vec2Si32 &operator*=(Si32 const &s) {
    x *= s;
    y *= s;
    return *this;
  }
  Vec2Si32 &operator*=(Vec2Si32  const &v) {
    x *= v.x;
    y *= v.y;
    return *this;
  }
  Vec2Si32 &operator/=(Si32 const &s) {
    x /= s;
    y /= s;
    return *this;
  }
  Vec2Si32 &operator/=(Vec2Si32  const &v) {
    x /= v.x;
    y /= v.y;
    return *this;
  }
  const bool operator== (const Vec2Si32 &v) const {
    return x == v.x && y == v.y;
  }
  const bool operator!= (const Vec2Si32 &v) const {
    return x != v.x || y != v.y;
  }

  Vec2Si32 xx() const {
    return Vec2Si32(x, x);
  }
  Vec2Si32 xy() const {
    return Vec2Si32(x, y);
  }
  Vec2Si32 yx() const {
    return Vec2Si32(y, x);
  }
  Vec2Si32 yy() const {
    return Vec2Si32(y, y);
  }

  Vec2Si32 xo() const {
    return Vec2Si32(x, 0);
  }
  Vec2Si32 ox() const {
    return Vec2Si32(0, x);
  }
  Vec2Si32 yo() const {
    return Vec2Si32(y, 0);
  }
  Vec2Si32 oy() const {
    return Vec2Si32(0, y);
  }
};

inline Vec2Si32 operator+(Vec2Si32  const &v, Si32 const &s) {
  return Vec2Si32(v.x + s, v.y + s);
}
inline Vec2Si32 operator+(Si32 const &s, Vec2Si32  const &v) {
  return Vec2Si32(s + v.x, s + v.y);
}
inline Vec2Si32 operator+(Vec2Si32  const &a, Vec2Si32  const &b) {
  return Vec2Si32(a.x + b.x, a.y + b.y);
}
inline Vec2Si32 operator-(Vec2Si32  const &v, Si32 const &s) {
  return Vec2Si32(v.x - s, v.y - s);
}
inline Vec2Si32 operator-(Si32 const &s, Vec2Si32  const &v) {
  return Vec2Si32(s - v.x, s - v.y);
}
inline Vec2Si32 operator-(Vec2Si32  const &a, Vec2Si32  const &b) {
  return Vec2Si32(a.x - b.x, a.y - b.y);
}
inline Vec2Si32 operator*(Vec2Si32  const &v, Si32 const &s) {
  return Vec2Si32(v.x * s, v.y * s);
}
inline Vec2Si32 operator*(Si32 const &s, Vec2Si32  const &v) {
  return Vec2Si32(s * v.x, s * v.y);
}
inline Vec2Si32 operator*(Vec2Si32 const &a, Vec2Si32 const &b) {
  return Vec2Si32(a.x * b.x, a.y * b.y);
}
inline Vec2Si32 operator/(Vec2Si32 const &v, Si32 const &s) {
  return Vec2Si32(v.x / s, v.y / s);
}
inline Vec2Si32 operator/(Si32 const &s, Vec2Si32  const &v) {
  return Vec2Si32(s / v.x, s / v.y);
}
inline Vec2Si32 operator/(Vec2Si32 const &a, Vec2Si32 const &b) {
  return Vec2Si32(a.x / b.x, a.y / b.y);
}


inline Vec2Si32 Max(const Vec2Si32 &a, const Vec2Si32 &b) {
  return Vec2Si32((a.x > b.x) ? a.x : b.x, (a.y > b.y) ? a.y : b.y);
}

inline Vec2Si32 Min(const Vec2Si32 &a, const Vec2Si32 &b) {
  return Vec2Si32((a.x < b.x) ? a.x : b.x, (a.y < b.y) ? a.y : b.y);
}

inline Vec2Si32 Min(const Vec2Si32 &v, Si32 mi) {  // NOLINT
  return Vec2Si32((v.x > mi) ? mi : v.x, (v.y > mi) ? mi : v.y);
}

inline Si64 LengthSquared(Vec2Si32 const &v) {
  return v.x * v.x + v.y * v.y;
}

inline Vec2Si32 Clamp(const Vec2Si32 &v, Si32 mi, Si32 ma) {
  return Vec2Si32((v.x < mi) ? mi : ((v.x > ma) ? ma : v.x),
    (v.y < mi) ? mi : ((v.y > ma) ? ma : v.y));
}
 
inline Vec2Si32 Clamp(const Vec2Si32 &v, Vec2Si32 mi, Vec2Si32 ma) {
  return Vec2Si32((v.x < mi.x) ? mi.x : ((v.x > ma.x) ? ma.x : v.x),
    (v.y < mi.y) ? mi.y : ((v.y > ma.y) ? ma.y : v.y));
}

inline bool IsInRange(const Vec2Si32 &v, const Vec2Si32 &mi, const Vec2Si32 &ma) {
  return (v.x >= mi.x && v.y >= mi.y && v.x <= ma.x && v.y <= ma.y);
}
/// @}


}  // namespace arctic

#endif  // ENGINE_VEC2SI32_H_
