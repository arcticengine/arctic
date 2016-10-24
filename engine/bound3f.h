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

#ifndef ENGINE_BOUND3F_H_
#define ENGINE_BOUND3F_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

struct Bound3F {
  union {
    struct {
      float min_x;
      float max_x;
      float min_y;
      float max_y;
      float min_z;
      float max_z;
    };
    float element[6];
  };

  Bound3F() {}

  explicit Bound3F(float mix, float max, float miy, float may,
    float miz, float maz) {
    min_x = mix;
    max_x = max;
    min_y = miy;
    max_y = may;
    min_z = miz;
    max_z = maz;
  }

  explicit Bound3F(float infi) {
    min_x = infi;
    max_x = -infi;
    min_y = infi;
    max_y = -infi;
    min_z = infi;
    max_z = -infi;
  }

  explicit Bound3F(float const *const v) {
    min_x = v[0];
    max_x = v[1];
    min_y = v[2];
    max_y = v[3];
    min_z = v[4];
    max_z = v[5];
  }

  explicit Bound3F(float const *const vmin, float const *const vmax) {
    min_x = vmin[0];
    max_x = vmax[0];
    min_y = vmin[1];
    max_y = vmax[1];
    min_z = vmin[2];
    max_z = vmax[2];
  }

  explicit Bound3F(const Vec3F &mi, const Vec3F &ma) {
    min_x = mi.x;
    max_x = ma.x;
    min_y = mi.y;
    max_y = ma.y;
    min_z = mi.z;
    max_z = ma.z;
  }

  float &operator[](Si32 i) {
    return element[i];
  }
  const float &operator[](Si32 i) const {
    return element[i];
  }
};

inline Bound3F Expand(const Bound3F &a, const Bound3F &b) {
  return Bound3F(a.min_x + b.min_x, a.max_x + b.max_x,
    a.min_y + b.min_y, a.max_y + b.max_y,
    a.min_z + b.min_z, a.max_z + b.max_z);
}

inline Bound3F Expand(const Bound3F &a, float const b) {
  return Bound3F(a.min_x - b, a.max_x + b,
    a.min_y - b, a.max_y + b,
    a.min_z - b, a.max_z + b);
}

inline Bound3F Include(const Bound3F &a, const Vec3F &p) {
  Bound3F res = Bound3F(
    (p.x < a.min_x) ? p.x : a.min_x,
    (p.x > a.max_x) ? p.x : a.max_x,
    (p.y < a.min_y) ? p.y : a.min_y,
    (p.y > a.max_y) ? p.y : a.max_y,
    (p.z < a.min_z) ? p.z : a.min_z,
    (p.z > a.max_z) ? p.z : a.max_z);

  return res;
}

inline Bound3F Include(const Bound3F &a, const Bound3F &b) {
  return Bound3F(fminf(a.min_x, b.min_x),
    fmaxf(a.max_x, b.max_x),
    fminf(a.min_y, b.min_y),
    fmaxf(a.max_y, b.max_y),
    fminf(a.min_z, b.min_z),
    fmaxf(a.max_z, b.max_z));
}

inline float Distance(const Bound3F &b, const Vec3F &p) {
  const Vec3F bc = 0.5f
    * Vec3F(b.max_x + b.min_x, b.max_y + b.min_y, b.max_z + b.min_z);
  const Vec3F br = 0.5f
    * Vec3F(b.max_x - b.min_x, b.max_y - b.min_y, b.max_z - b.min_z);

  const Vec3F d = abs(p - bc) - br;
  return fminf(fmaxf(d.x, fmaxf(d.y, d.z)), 0.0f)
    + Length(max(d, 0.0f));  // NOLINT
}

inline bool Contains(Bound3F const &b, Vec3F const &p) {
  if (p.x < b.min_x) {
    return false;
  }
  if (p.y < b.min_y) {
    return false;
  }
  if (p.z < b.min_z) {
    return false;
  }
  if (p.x > b.max_x) {
    return false;
  }
  if (p.y > b.max_y) {
    return false;
  }
  if (p.z > b.max_z) {
    return false;
  }
  return true;
}

#if 0
// 0 = a and b are disjoint
// 1 = a and b intersect
// 2 = b is fully contained in a
// 3 = a is fully contained in b
inline Si32 Contains(bound3 const &a, bound3 const &b) {
  Si32 nBinA = containsp(a, vec3(b.mMinX, b.mMinY, b.mMinZ)) +
    containsp(a, vec3(b.mMaxX, b.mMinY, b.mMinZ)) +
    containsp(a, vec3(b.mMinX, b.mMaxY, b.mMinZ)) +
    containsp(a, vec3(b.mMaxX, b.mMaxY, b.mMinZ)) +
    containsp(a, vec3(b.mMinX, b.mMinY, b.mMaxZ)) +
    containsp(a, vec3(b.mMaxX, b.mMinY, b.mMaxZ)) +
    containsp(a, vec3(b.mMinX, b.mMaxY, b.mMaxZ)) +
    containsp(a, vec3(b.mMaxX, b.mMaxY, b.mMaxZ));

  Si32 nAinB = containsp(b, vec3(a.mMinX, a.mMinY, a.mMinZ)) +
    containsp(b, vec3(a.mMaxX, a.mMinY, a.mMinZ)) +
    containsp(b, vec3(a.mMinX, a.mMaxY, a.mMinZ)) +
    containsp(b, vec3(a.mMaxX, a.mMaxY, a.mMinZ)) +
    containsp(b, vec3(a.mMinX, a.mMinY, a.mMaxZ)) +
    containsp(b, vec3(a.mMaxX, a.mMinY, a.mMaxZ)) +
    containsp(b, vec3(a.mMinX, a.mMaxY, a.mMaxZ)) +
    containsp(b, vec3(a.mMaxX, a.mMaxY, a.mMaxZ));

  if (nAinB == 0 && nBinA == 0) {
    return 0;
  }
  if (nAinB != 0 && nBinA != 0) {
    return 1;
  }
  if (nAinB == 0 && nBinA != 0) {
    return 2;
  }
  /*if( nAinB!=0 && nBinA==0 )*/ return 3;
}
#endif  // 0

// 0 if they are disjoint
// 1 if they intersect
inline Si32 Overlap(Bound3F const &a, Bound3F const &b) {
  if (a.max_x < b.min_x) {
    return 0;
  }
  if (a.min_x > b.max_x) {
    return 0;
  }
  if (a.max_y < b.min_y) {
    return 0;
  }
  if (a.min_y > b.max_y) {
    return 0;
  }
  if (a.max_z < b.min_z) {
    return 0;
  }
  if (a.min_z > b.max_z) {
    return 0;
  }
  return 1;
}

inline Bound3F Compute(const Vec3F *const p, const Si32 num) {
  Bound3F res = Bound3F(p[0].x, p[0].x,
    p[0].y, p[0].y,
    p[0].z, p[0].z);

  for (Si32 k = 1; k < num; k++) {
    res.min_x = (p[k].x < res.min_x) ? p[k].x : res.min_x;
    res.max_x = (p[k].x > res.max_x) ? p[k].x : res.max_x;
    res.min_y = (p[k].y < res.min_y) ? p[k].y : res.min_y;
    res.max_y = (p[k].y > res.max_y) ? p[k].y : res.max_y;
    res.min_z = (p[k].z < res.min_z) ? p[k].z : res.min_z;
    res.max_z = (p[k].z > res.max_z) ? p[k].z : res.max_z;
  }

  return res;
}

inline float Diagonal(Bound3F const &bbox) {
  const float dx = bbox.max_x - bbox.min_x;
  const float dy = bbox.max_y - bbox.min_y;
  const float dz = bbox.max_z - bbox.min_z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

inline float Volume(Bound3F const &bbox) {
  const float dx = bbox.max_x - bbox.min_x;
  const float dy = bbox.max_y - bbox.min_y;
  const float dz = bbox.max_z - bbox.min_z;
  return dx * dy * dz;
}

inline Vec3F GetCenter(Bound3F const &bbox) {
  return Vec3F(0.5f * (bbox.min_x + bbox.max_x),
    0.5f * (bbox.min_y + bbox.max_y),
    0.5f * (bbox.min_z + bbox.max_z));
}

inline Vec3F GetRadius(Bound3F const &bbox) {
  return Vec3F(0.5f * (bbox.max_x - bbox.min_x),
    0.5f * (bbox.max_y - bbox.min_y),
    0.5f * (bbox.max_z - bbox.min_z));
}

inline Bound3F Transform(Bound3F const &bbox, const Mat44F &m) {
  Vec3F p0 = Transform(m, Vec3F(bbox.min_x, bbox.min_y, bbox.min_z));
  Vec3F p1 = Transform(m, Vec3F(bbox.max_x, bbox.min_y, bbox.min_z));
  Vec3F p2 = Transform(m, Vec3F(bbox.min_x, bbox.max_y, bbox.min_z));
  Vec3F p3 = Transform(m, Vec3F(bbox.max_x, bbox.max_y, bbox.min_z));
  Vec3F p4 = Transform(m, Vec3F(bbox.min_x, bbox.min_y, bbox.max_z));
  Vec3F p5 = Transform(m, Vec3F(bbox.max_x, bbox.min_y, bbox.max_z));
  Vec3F p6 = Transform(m, Vec3F(bbox.min_x, bbox.max_y, bbox.max_z));
  Vec3F p7 = Transform(m, Vec3F(bbox.max_x, bbox.max_y, bbox.max_z));

  Bound3F res = Bound3F(p0, p0);
  res = Include(res, p1);
  res = Include(res, p2);
  res = Include(res, p3);
  res = Include(res, p4);
  res = Include(res, p5);
  res = Include(res, p6);
  res = Include(res, p7);
  return res;
}

}  // namespace arctic

#endif  // ENGINE_BOUND3F_H_
