// The MIT License (MIT)
//
// Copyright (c) 2026 Huldra
//
// Permission is hereby granted, free of charge to any person obtaining a copy
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

#ifndef ENGINE_SPHERE_VS_TRIANGLE_H_
#define ENGINE_SPHERE_VS_TRIANGLE_H_

#include "engine/arctic_types.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_math
/// @{

/// A sphere that may be moving. Velocity is the displacement over the interval
/// being tested, so a hit time of 1 is the end of the move.
struct MovingSphere {
  Vec3F center;
  float radius = 0.0f;
  Vec3F velocity;
  float radius_sq = 0.0f;
  float velocity_sq = 0.0f;
  float velocity_length = 0.0f;
  float velocity_length_plus_radius = 0.0f;
  float inv_velocity_sq = 0.0f;

  MovingSphere() {}
  MovingSphere(const Vec3F &o, float r, const Vec3F &v);
};

/// A triangle with the edge and plane data the swept-sphere test needs, plus a
/// bounding sphere used to reject distant queries. Set() returns false when
/// the three points do not span a plane.
struct CollisionTriangle {
  Vec3F a;
  Vec3F b;
  Vec3F c;
  Vec3F n;
  float plane_d = 0.0f;
  Vec3F ab;
  Vec3F ac;
  Vec3F bc;
  Vec3F ab_n;
  Vec3F ac_n;
  Vec3F bc_n;
  float ab_length = 0.0f;
  float ac_length = 0.0f;
  float bc_length = 0.0f;
  float inv_ab_length_sq = 0.0f;
  float inv_ac_length_sq = 0.0f;
  float inv_bc_length_sq = 0.0f;
  Vec3F ab_x_ac;
  Vec3F bc_x_ba;
  Vec3F ac_x_ab;
  Vec3F bound_center;
  float bound_radius = 0.0f;
  float bound_radius_sq = 0.0f;

  bool Set(const Vec3F &pa, const Vec3F &pb, const Vec3F &pc);
  float SignedDistance(const Vec3F &p) const;
  Vec3F Project(const Vec3F &p) const;
  bool ContainsPoint(const Vec3F &p) const;
  Vec3F ClosestPoint(const Vec3F &p) const;
};

/// time is in [0, 1] along the sphere's velocity. point is on the triangle.
/// normal points from the triangle toward the sphere.
struct SphereTriangleHit {
  bool hit = false;
  float time = 1.0f;
  Vec3F point;
  Vec3F normal;
};

bool SphereOverlapsTriangle(const MovingSphere &sph,
    const CollisionTriangle &tri);
SphereTriangleHit SphereTriangleContact(const MovingSphere &sph,
    const CollisionTriangle &tri);
SphereTriangleHit SweptSphereVsTriangle(const MovingSphere &sph,
    const CollisionTriangle &tri);
SphereTriangleHit SweptSphereVsTriangles(const MovingSphere &sph,
    const CollisionTriangle *tris, Si32 count);

/// True when the open segment p0-p1 goes from one side of the triangle's
/// plane to the other and the plane hit lies on the filled triangle
/// (interior or edge). A center that starts and ends on the same side, or
/// that misses the polygon, is not a pierce.
bool LineSegmentPiercesTriangle(const Vec3F &p0, const Vec3F &p1,
    const CollisionTriangle &tri, float *out_t, Vec3F *out_p);

/// A displacement that went through a triangle the sphere must collide with:
/// the center crossed the filled triangle, or the sphere was free at the
/// start and overlapping at the end without SweptSphereVsTriangle stopping
/// the move. Sitting on a face and sliding along it is not a pass-through.
struct SpherePassThrough {
  bool passed = false;
  bool center_pierce = false;
  bool entered_without_hit = false;
  float pierce_t = 1.0f;
  Vec3F pierce_p;
  bool start_overlap = false;
  bool end_overlap = false;
  SphereTriangleHit sweep;
};

SpherePassThrough ClassifySpherePassThrough(const MovingSphere &sph,
    const CollisionTriangle &tri);

/// @}

}  // namespace arctic

#endif  // ENGINE_SPHERE_VS_TRIANGLE_H_
