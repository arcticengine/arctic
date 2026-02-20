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

#ifndef ENGINE_VEC2D_H_
#define ENGINE_VEC2D_H_

#include <cmath>
#include <iosfwd>
#include "engine/arctic_types.h"
#include "engine/vec2si32.h"

namespace arctic {

/// @addtogroup global_math
/// @{
struct Vec2D {
  union {
    struct {
      double x;
      double y;
    };
    double element[2];
  };

  Vec2D() {}

  explicit Vec2D(double a) {
    x = a;
    y = a;
  }

  explicit Vec2D(double a, double b) {
    x = a;
    y = b;
  }

  explicit Vec2D(double const *const v) {
    x = v[0];
    y = v[1];
  }

  explicit Vec2D(const Vec2Si32 &v) {
    x = static_cast<double>(v.x);
    y = static_cast<double>(v.y);
  }

  double &operator[](Si32 i) {
    return element[i];
  }
  const double &operator[](Si32 i) const {
    return element[i];
  }

  Vec2D &operator= (Vec2D const &v) {
    x = v.x;
    y = v.y;
    return *this;
  }
  Vec2D &operator+=(double const &s) {
    x += s;
    y += s;
    return *this;
  }
  Vec2D &operator+=(Vec2D const &v) {
    x += v.x;
    y += v.y;
    return *this;
  }
  Vec2D &operator-=(double const &s) {
    x -= s;
    y -= s;
    return *this;
  }
  Vec2D &operator-=(Vec2D const &v) {
    x -= v.x;
    y -= v.y;
    return *this;
  }
  Vec2D &operator*=(double const &s) {
    x *= s;
    y *= s;
    return *this;
  }
  Vec2D &operator*=(Vec2D const &v) {
    x *= v.x;
    y *= v.y;
    return *this;
  }
  Vec2D &operator/=(double const &s) {
    x /= s;
    y /= s;
    return *this;
  }
  Vec2D &operator/=(Vec2D const &v) {
    x /= v.x;
    y /= v.y;
    return *this;
  }

  Vec2D xx() const {
    return Vec2D(x, x);
  }
  Vec2D xy() const {
    return Vec2D(x, y);
  }
  Vec2D yx() const {
    return Vec2D(y, x);
  }
  Vec2D yy() const {
    return Vec2D(y, y);
  }

  friend std::ostream& operator<<(std::ostream& os, const Vec2D& vec);
};


inline Vec2D operator+(Vec2D const &v, double const &s) {
  return Vec2D(v.x + s, v.y + s);
}
inline Vec2D operator+(double const &s, Vec2D const &v) {
  return Vec2D(s + v.x, s + v.y);
}
inline Vec2D operator+(Vec2D const &a, Vec2D const &b) {
  return Vec2D(a.x + b.x, a.y + b.y);
}
inline Vec2D operator-(Vec2D const &v, double const &s) {
  return Vec2D(v.x - s, v.y - s);
}
inline Vec2D operator-(double const &s, Vec2D const &v) {
  return Vec2D(s - v.x, s - v.y);
}
inline Vec2D operator-(Vec2D const &a, Vec2D const &b) {
  return Vec2D(a.x - b.x, a.y - b.y);
}
inline Vec2D operator*(Vec2D const &v, double const &s) {
  return Vec2D(v.x * s, v.y * s);
}
inline Vec2D operator*(double const &s, Vec2D const &v) {
  return Vec2D(s * v.x, s * v.y);
}
inline Vec2D operator*(Vec2D const &a, Vec2D const &b) {
  return Vec2D(a.x * b.x, a.y * b.y);
}
inline Vec2D operator/(Vec2D const &v, double const &s) {
  return Vec2D(v.x / s, v.y / s);
}
inline Vec2D operator/(double const &s, Vec2D const &v) {
  return Vec2D(s / v.x, s / v.y);
}
inline Vec2D operator/(Vec2D const &a, Vec2D const &b) {
  return Vec2D(a.x / b.x, a.y / b.y);
}

inline Vec2D floor(Vec2D const &v) {
  return Vec2D(std::floor(v.x), std::floor(v.y));
}

inline Vec2D Normalize(Vec2D const &v) {
  double const m2 = v.x * v.x + v.y * v.y;
  double const im = 1.0 / std::sqrt(m2);
  return Vec2D(v.x * im, v.y * im);
}

inline Vec2D Mix(Vec2D const &a, Vec2D const &b, double const f) {
  return Vec2D(a.x * (1.0 - f) + f * b.x,
    a.y * (1.0 - f) + f * b.y);
}


inline double Length(Vec2D const &v) {
  return std::sqrt(v.x * v.x + v.y * v.y);
}

inline double LengthSquared(Vec2D const &v) {
  return v.x * v.x + v.y * v.y;
}

inline double Dot(Vec2D const &a, Vec2D const &b) {
  return a.x * b.x + a.y * b.y;
}

inline double Distance(Vec2D const &a, Vec2D const &b) {
  return Length(a - b);
}

inline Vec2D Perpendicular(Vec2D const &v) {
  return Vec2D(v.y, -v.x);
}

inline Vec2D FromPolar(const double a) {
  return Vec2D(std::cos(a), std::sin(a));
}

inline double InverseLength(Vec2D const &v) {
  return 1.0f / std::sqrt(v.x * v.x + v.y * v.y);
}

inline Vec2D sin(const Vec2D &v) {
  return Vec2D(std::sin(v.x), std::sin(v.y));
}

inline Vec2D cos(const Vec2D &v) {
  return Vec2D(std::cos(v.x), std::cos(v.y));
}

inline Vec2D sqrt(const Vec2D &v) {
  return Vec2D(std::sqrt(v.x), std::sqrt(v.y));
}

inline bool IsInRange(const Vec2D &v, const Vec2D &mi, const Vec2D &ma) {
  return (v.x >= mi.x && v.y >= mi.y && v.x <= ma.x && v.y <= ma.y);
}

/// @}

}  // namespace arctic

#endif  // ENGINE_VEC2D_H_
