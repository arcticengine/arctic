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

#ifndef ENGINE_VEC2SI32_H_
#define ENGINE_VEC2SI32_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

struct Vec2Si32 {
  union {
    struct {
      Si32 x;
      Si32 y;
    };
    Si32 element[2];
  };

  Vec2Si32() {}

  explicit Vec2Si32(Si32 a, Si32 b) {
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

inline Vec2Si32 Clamp(const Vec2Si32 &v, Si32 mi, Si32 ma) {
  return Vec2Si32((v.x < mi) ? mi : ((v.x > ma) ? ma : v.x),
    (v.y < mi) ? mi : ((v.y > ma) ? ma : v.y));
}


}  // namespace arctic

#endif  // ENGINE_VEC2SI32_H_
