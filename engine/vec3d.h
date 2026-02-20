// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2022 Huldra
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

#ifndef ENGINE_VEC3D_H_
#define ENGINE_VEC3D_H_

#include <cmath>
#include "engine/arctic_types.h"
#include "engine/vec2d.h"
#include "engine/vec3si32.h"

namespace arctic {

/// @addtogroup global_math
/// @{
struct Vec3D {
  union {
    struct {
      double x;
      double y;
      double z;
    };
    double element[3];
  };

  Vec3D() {}

  explicit Vec3D(double a) {
    x = a;
    y = a;
    z = a;
  }

  explicit Vec3D(double a, double b, double c) {
    x = a;
    y = b;
    z = c;
  }

  explicit Vec3D(const double *v) {
    x = v[0];
    y = v[1];
    z = v[2];
  }

  explicit Vec3D(Vec2D const &v, double s) {
    x = v.x;
    y = v.y;
    z = s;
  }

  explicit Vec3D(double s, Vec2D const &v) {
    x = s;
    y = v.x;
    z = v.y;
  }

  explicit Vec3D(const Vec3Si32 &v) {
    x = static_cast<double>(v.x);
    y = static_cast<double>(v.y);
    z = static_cast<double>(v.z);
  }

  double &operator[](Si32 i) {
    return element[i];
  }
  const double &operator[](Si32 i) const {
    return element[i];
  }

  Vec3D &operator =(Vec3D  const &v) {
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
  }
  Vec3D &operator+=(double const &s) {
    x += s;
    y += s;
    z += s;
    return *this;
  }
  Vec3D &operator+=(Vec3D  const &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  Vec3D &operator-=(double const &s) {
    x -= s;
    y -= s;
    z -= s;
    return *this;
  }
  Vec3D &operator-=(Vec3D  const &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }
  Vec3D &operator*=(double const &s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }
  Vec3D &operator*=(Vec3D  const &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
  }
  Vec3D &operator/=(double const &s) {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }
  Vec3D &operator/=(Vec3D  const &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    return *this;
  }

  Vec2D xx() const {
    return Vec2D(x, x);
  }
  Vec2D xy() const {
    return Vec2D(x, y);
  }
  Vec2D xz() const {
    return Vec2D(x, z);
  }
  Vec2D yx() const {
    return Vec2D(y, x);
  }
  Vec2D yy() const {
    return Vec2D(y, y);
  }
  Vec2D yz() const {
    return Vec2D(y, z);
  }
  Vec2D zx() const {
    return Vec2D(z, x);
  }
  Vec2D zy() const {
    return Vec2D(z, y);
  }
  Vec2D zz() const {
    return Vec2D(z, z);
  }

  Vec3D xxx() const {
    return Vec3D(x, x, x);
  }
  Vec3D xxy() const {
    return Vec3D(x, x, y);
  }
  Vec3D xxz() const {
    return Vec3D(x, x, z);
  }
  Vec3D xyx() const {
    return Vec3D(x, y, x);
  }
  Vec3D xyy() const {
    return Vec3D(x, y, y);
  }
  Vec3D xyz() const {
    return Vec3D(x, y, z);
  }
  Vec3D xzx() const {
    return Vec3D(x, z, x);
  }
  Vec3D xzy() const {
    return Vec3D(x, z, y);
  }
  Vec3D xzz() const {
    return Vec3D(x, z, z);
  }
  Vec3D yxx() const {
    return Vec3D(y, x, x);
  }
  Vec3D yxy() const {
    return Vec3D(y, x, y);
  }
  Vec3D yxz() const {
    return Vec3D(y, x, z);
  }
  Vec3D yyx() const {
    return Vec3D(y, y, x);
  }
  Vec3D yyy() const {
    return Vec3D(y, y, y);
  }
  Vec3D yyz() const {
    return Vec3D(y, y, z);
  }
  Vec3D yzx() const {
    return Vec3D(y, z, x);
  }
  Vec3D yzy() const {
    return Vec3D(y, z, y);
  }
  Vec3D yzz() const {
    return Vec3D(y, z, z);
  }
  Vec3D zxx() const {
    return Vec3D(z, x, x);
  }
  Vec3D zxy() const {
    return Vec3D(z, x, y);
  }
  Vec3D zxz() const {
    return Vec3D(z, x, z);
  }
  Vec3D zyx() const {
    return Vec3D(z, y, x);
  }
  Vec3D zyy() const {
    return Vec3D(z, y, y);
  }
  Vec3D zyz() const {
    return Vec3D(z, y, z);
  }
  Vec3D zzx() const {
    return Vec3D(z, z, x);
  }
  Vec3D zzy() const {
    return Vec3D(z, z, y);
  }
  Vec3D zzz() const {
    return Vec3D(z, z, z);
  }
};

inline Vec3D operator+(const Vec3D &v, double const &s) {
  return Vec3D(v.x + s, v.y + s, v.z + s);
}
inline Vec3D operator+(double const &s, Vec3D  const &v) {
  return Vec3D(s + v.x, s + v.y, s + v.z);
}
inline Vec3D operator+(Vec3D  const &a, Vec3D  const &b) {
  return Vec3D(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline Vec3D operator-(Vec3D  const &v, double const &s) {
  return Vec3D(v.x - s, v.y - s, v.z - s);
}
inline Vec3D operator-(double const &s, Vec3D  const &v) {
  return Vec3D(s - v.x, s - v.y, s - v.z);
}
inline Vec3D operator-(Vec3D  const &a, Vec3D  const &b) {
  return Vec3D(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline Vec3D operator*(Vec3D  const &v, double const &s) {
  return Vec3D(v.x * s, v.y * s, v.z * s);
}
inline Vec3D operator*(double const &s, Vec3D  const &v) {
  return Vec3D(s * v.x, s * v.y, s * v.z);
}
inline Vec3D operator*(Vec3D  const &a, Vec3D  const &b) {
  return Vec3D(a.x * b.x, a.y * b.y, a.z * b.z);
}
inline Vec3D operator/(Vec3D  const &v, double const &s) {
  return Vec3D(v.x / s, v.y / s, v.z / s);
}
inline Vec3D operator/(double const &s, Vec3D  const &v) {
  return Vec3D(s / v.x, s / v.y, s / v.z);
}
inline Vec3D operator/(Vec3D  const &a, Vec3D  const &b) {
  return Vec3D(a.x / b.x, a.y / b.y, a.z / b.z);
}


inline Vec3D floor(Vec3D const &v) {
  return Vec3D(std::floor(v.x), std::floor(v.y), std::floor(v.z));
}

inline Vec3D Normalize(Vec3D const &v) {
  double const m2 = v.x * v.x + v.y * v.y + v.z * v.z;
  double const im = 1.0 / std::sqrt(m2);
  return Vec3D(v.x * im, v.y * im, v.z * im);
}

inline Vec3D NormalizeSafe(Vec3D const &v) {
  double const m2 = v.x * v.x + v.y * v.y + v.z * v.z;
  if (m2 <= 0.000000001f) {
    return Vec3D(0.0);
  }
  double const im = 1.0 / std::sqrt(m2);
  return Vec3D(v.x * im, v.y * im, v.z * im);
}

inline Vec3D Mix(Vec3D const &a, Vec3D const &b, double const f) {
  return Vec3D(a.x * (1.0 - f) + f * b.x,
    a.y * (1.0 - f) + f * b.y,
    a.z * (1.0 - f) + f * b.z);
}

inline Vec3D Cross(Vec3D const &a, Vec3D const &b) {
  return Vec3D(a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x);
}

inline double Length(Vec3D const &v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline double LengthSquared(Vec3D const &v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline double InverseLength(Vec3D const &v) {
  return 1.0 / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline double Dot(Vec3D const &a, Vec3D const &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double Distance(Vec3D const &a, Vec3D const &b) {
  return Length(a - b);
}

inline void BuildBase(const Vec3D &n, Vec3D *uu, Vec3D *vv) {
  Vec3D up;
  if (std::abs(n.z) < 0.9f) {
    up.x = 0.0;
    up.y = 0.0;
    up.z = 1.0;
  } else {
    up.x = 1.0;
    up.y = 0.0;
    up.z = 0.0;
  }
  *vv = Normalize(Cross(n, up));
  *uu = Normalize(Cross(*vv, n));
}


inline Vec3D Orientate(const Vec3D &v, const Vec3D &dir) {
  Vec3D res = v;
  const double kk = Dot(dir, v);
  if (kk < 0.0) {
    res -= 2.0 * dir * kk;
  }
  return res;
}

inline Vec3D pow(const Vec3D &v, const double f) {
  return Vec3D(std::pow(v.x, f), std::pow(v.y, f), std::pow(v.z, f));
}

inline Vec3D pow(const Vec3D &v, const Vec3D &f) {
  return Vec3D(std::pow(v.x, f.x), std::pow(v.y, f.y), std::pow(v.z, f.z));
}

inline Vec3D sin(const Vec3D &v) {
  return Vec3D(std::sin(v.x), std::sin(v.y), std::sin(v.z));
}

inline Vec3D cos(const Vec3D &v) {
  return Vec3D(std::cos(v.x), std::cos(v.y), std::cos(v.z));
}

inline Vec3D mod(const Vec3D &v, double s) {
  return Vec3D(fmod(v.x, s), fmod(v.y, s), fmod(v.z, s));
}

inline Vec3D sqrt(const Vec3D &v) {
  return Vec3D(std::sqrt(v.x), std::sqrt(v.y), std::sqrt(v.z));
}

inline Vec3D Min(const Vec3D &v, double mi) {
  return Vec3D((v.x < mi) ? v.x : mi,
    (v.y < mi) ? v.y : mi,
    (v.z < mi) ? v.z : mi);
}
inline Vec3D Max(const Vec3D &v, double ma) {
  return Vec3D((v.x > ma) ? v.x : ma,
    (v.y > ma) ? v.y : ma,
    (v.z > ma) ? v.z : ma);
}
inline Vec3D Clamp(const Vec3D &v, double mi, double ma) {
  return Max(Min(v, ma), mi);
}
inline Vec3D Clamp01(const Vec3D &v) {
  return Max(Min(v, 1.0), 0.0);
}
inline Vec3D Clamp1(const Vec3D &v) {
  return Max(Min(v, 1.0), -1.0);  // NOLINT
}
inline Vec3D Abs(const Vec3D &v) {
  return Vec3D(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

inline Vec3D SmoothStep(double a, double b, const Vec3D &v) {
  Vec3D x = Clamp01((v - Vec3D(a)) / (b - a));
  return x * x * (3.0 - 2.0 * x);
}

inline double MaxComp(const Vec3D &v) {
  return (v.x > v.y) ? ((v.x > v.z) ? v.x : v.z) : ((v.y > v.z) ? v.y : v.z);
}
/// @}


}  // namespace arctic

#endif  // ENGINE_VEC3D_H_
