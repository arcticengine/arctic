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

#ifndef ENGINE_VEC3SI32_H_
#define ENGINE_VEC3SI32_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

struct Vec3Si32 {
  union {
    struct {
      Si32 x;
      Si32 y;
      Si32 z;
    };
    Si32 element[3];
  };

  Vec3Si32() {}

  explicit Vec3Si32(Si32 a, Si32 b, Si32 c) {
    x = a;
    y = b;
    z = c;
  }
  explicit Vec3Si32(Si32 s) {
    x = s;
    y = s;
    z = s;
  }
  explicit Vec3Si32(const Vec3F &v) {
    x = (Si32)v.x;
    y = (Si32)v.y;
    z = (Si32)v.z;
  }

  Si32 &operator[](Si32 i) {
    return element[i];
  }
  const Si32 &operator[](Si32 i) const {
    return element[i];
  }

  Vec3Si32 &operator =(Vec3Si32 const &v) {
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
  }
  Vec3Si32 &operator+=(Si32 const &s) {
    x += s;
    y += s;
    return *this;
  }
  Vec3Si32 &operator+=(Vec3Si32 const &v) {
    x += v.x;
    y += v.y;
    return *this;
  }
  Vec3Si32 &operator-=(Si32 const &s) {
    x -= s;
    y -= s;
    return *this;
  }
  Vec3Si32 &operator-=(Vec3Si32 const &v) {
    x -= v.x;
    y -= v.y;
    return *this;
  }
  Vec3Si32 &operator*=(Si32 const &s) {
    x *= s;
    y *= s;
    return *this;
  }
  Vec3Si32 &operator*=(Vec3Si32 const &v) {
    x *= v.x;
    y *= v.y;
    return *this;
  }
  Vec3Si32 &operator/=(Si32 const &s) {
    x /= s;
    y /= s;
    return *this;
  }
  Vec3Si32 &operator/=(Vec3Si32 const &v) {
    x /= v.x;
    y /= v.y;
    return *this;
  }

  const bool operator== (const Vec3Si32 &v) const {
    return x == v.x && y == v.y && z == v.z;
  }
  const bool operator!= (const Vec3Si32 &v) const {
    return x != v.x || y != v.y || z != v.z;
  }
};

inline Vec3Si32 operator+(Vec3Si32 const &v, Si32 const &s) {
  return Vec3Si32(v.x + s, v.y + s, v.z + s);
}
inline Vec3Si32 operator+(Si32 const &s, Vec3Si32 const &v) {
  return Vec3Si32(s + v.x, s + v.y, s + v.z);
}
inline Vec3Si32 operator+(Vec3Si32 const &a, Vec3Si32 const &b) {
  return Vec3Si32(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline Vec3Si32 operator-(Vec3Si32 const &v, Si32 const &s) {
  return Vec3Si32(v.x - s, v.y - s, v.z - s);
}
inline Vec3Si32 operator-(Si32 const &s, Vec3Si32 const &v) {
  return Vec3Si32(s - v.x, s - v.y, s - v.z);
}
inline Vec3Si32 operator-(Vec3Si32 const &a, Vec3Si32 const &b) {
  return Vec3Si32(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline Vec3Si32 operator*(Vec3Si32 const &v, Si32 const &s) {
  return Vec3Si32(v.x * s, v.y * s, v.z * s);
}
inline Vec3Si32 operator*(Si32 const &s, Vec3Si32 const &v) {
  return Vec3Si32(s * v.x, s * v.y, s * v.z);
}
inline Vec3Si32 operator*(Vec3Si32 const &a, Vec3Si32 const &b) {
  return Vec3Si32(a.x * b.x, a.y * b.y, a.z * b.z);
}
inline Vec3Si32 operator/(Vec3Si32 const &v, Si32 const &s) {
  return Vec3Si32(v.x / s, v.y / s, v.z / s);
}
inline Vec3Si32 operator/(Si32 const &s, Vec3Si32 const &v) {
  return Vec3Si32(s / v.x, s / v.y, s / v.z);
}
inline Vec3Si32 operator/(Vec3Si32 const &a, Vec3Si32 const &b) {
  return Vec3Si32(a.x / b.x, a.y / b.y, a.z / b.z);
}

inline Vec3Si32 min(const Vec3Si32 &v, Si32 mi) {
  return Vec3Si32((v.x > mi) ? mi : v.x,
    (v.y > mi) ? mi : v.y,
    (v.z > mi) ? mi : v.z);
}

inline Vec3Si32 max(const Vec3Si32 &v, Si32 ma) {  // NOLINT
  return Vec3Si32((v.x < ma) ? ma : v.x,
    (v.y < ma) ? ma : v.y,
    (v.z < ma) ? ma : v.z);
}

inline Vec3Si32 Clamp(const Vec3Si32 &v, Si32 mi, Si32 ma) {
  return Vec3Si32((v.x < mi) ? mi : ((v.x > ma) ? ma : v.x),
    (v.y < mi) ? mi : ((v.y > ma) ? ma : v.y),
    (v.z < mi) ? mi : ((v.z > ma) ? ma : v.z));
}


}  // namespace arctic

#endif  // ENGINE_VEC3SI32_H_
