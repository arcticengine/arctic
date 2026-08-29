// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/sphere_vs_triangle.h"

#include <cmath>

namespace arctic {

namespace {

const float kEps = 1e-8f;

bool LowestRoot(float a, float b, float c, float max_t, float *out_t) {
  if (std::fabs(a) < kEps) {
    if (std::fabs(b) < kEps) {
      return false;
    }
    float t = -c / b;
    if (t >= 0.0f && t <= max_t) {
      *out_t = t;
      return true;
    }
    return false;
  }
  float disc = b * b - 4.0f * a * c;
  if (disc < 0.0f) {
    return false;
  }
  float sqrt_d = std::sqrt(disc);
  float inv = 0.5f / a;
  float r1 = (-b - sqrt_d) * inv;
  float r2 = (-b + sqrt_d) * inv;
  if (r1 > r2) {
    float tmp = r1;
    r1 = r2;
    r2 = tmp;
  }
  if (r1 >= 0.0f && r1 <= max_t) {
    *out_t = r1;
    return true;
  }
  if (r2 >= 0.0f && r2 <= max_t) {
    *out_t = r2;
    return true;
  }
  return false;
}

Vec3F PerpToLine(const Vec3F &x, const Vec3F &origin, const Vec3F &dir,
    float inv_dir_sq) {
  Vec3F ax = x - origin;
  Vec3F ap = dir * (Dot(ax, dir) * inv_dir_sq);
  return ap - ax;
}

void KeepEarlier(SphereTriangleHit *best, const SphereTriangleHit &cand) {
  if (!cand.hit) {
    return;
  }
  if (!best->hit || cand.time < best->time) {
    *best = cand;
  }
}

// The quadratic a*t^2 + b*t + c a vertex/edge test hands to LowestRoot has
// c == current offset^2 - radius^2 and b == that offset's derivative at
// t=0. Once the sphere already rests exactly radius away from a vertex or
// edge, as it does on the frame right after UnstickSphere places it there,
// c sits at or just below zero from float noise. LowestRoot then always
// has a root pinned near t=0 regardless of which way the sphere is moving:
// it is the entering crossing only when the offset is actually shrinking
// (b < 0). When b is flat or growing that near-zero root is the *exiting*
// crossing of the old contact, and accepting it as a fresh hit re-triggers
// the same feature every following frame even while the sphere slides
// safely past it, which read as the craft snagging on a rock corner and
// stalling in place with the throttle unable to move it an inch.
bool ApproachingFeature(float b, float c, float radius, float speed) {
  if (c > 0.0f) {
    return true;
  }
  return b < -2.0e-3f * radius * speed;
}

SphereTriangleHit HitPoint(const MovingSphere &sph, const Vec3F &p) {
  SphereTriangleHit miss;
  Vec3F op = p - sph.center;
  float c = LengthSquared(op) - sph.radius_sq;
  float b = -2.0f * Dot(op, sph.velocity);
  if (!ApproachingFeature(b, c, sph.radius, sph.velocity_length)) {
    return miss;
  }
  float t = 0.0f;
  if (!LowestRoot(sph.velocity_sq, b, c, 1.0f, &t)) {
    return miss;
  }
  SphereTriangleHit hit;
  hit.hit = true;
  hit.time = t;
  hit.point = p;
  hit.normal = sph.velocity * t - op;
  return hit;
}

SphereTriangleHit HitSegment(const MovingSphere &sph, const Vec3F &origin,
    const Vec3F &dir_n, float dir_length) {
  SphereTriangleHit miss;
  Vec3F ao = sph.center - origin;
  Vec3F m = dir_n * Dot(dir_n, ao) - ao;
  Vec3F n = dir_n * Dot(dir_n, sph.velocity) - sph.velocity;
  float c = LengthSquared(m) - sph.radius_sq;
  float b = 2.0f * Dot(m, n);
  if (!ApproachingFeature(b, c, sph.radius, sph.velocity_length)) {
    return miss;
  }
  float t = 0.0f;
  if (!LowestRoot(LengthSquared(n), b, c, 1.0f, &t)) {
    return miss;
  }
  Vec3F at = sph.center + sph.velocity * t - origin;
  float along = Dot(dir_n, at);
  if (along <= 0.0f || along >= dir_length) {
    return miss;
  }
  Vec3F ap = dir_n * along;
  SphereTriangleHit hit;
  hit.hit = true;
  hit.time = t;
  hit.point = origin + ap;
  hit.normal = sph.velocity * t - (ap - ao);
  return hit;
}

}  // namespace

MovingSphere::MovingSphere(const Vec3F &o, float r, const Vec3F &v) {
  center = o;
  radius = (r > 0.0f) ? r : 0.0f;
  radius_sq = radius * radius;
  velocity = v;
  velocity_sq = LengthSquared(velocity);
  velocity_length = std::sqrt(velocity_sq);
  velocity_length_plus_radius = velocity_length + radius;
  if (velocity_sq > kEps) {
    inv_velocity_sq = 1.0f / velocity_sq;
  } else {
    inv_velocity_sq = 0.0f;
  }
}

bool CollisionTriangle::Set(const Vec3F &pa, const Vec3F &pb,
    const Vec3F &pc) {
  a = pa;
  b = pb;
  c = pc;
  ab = b - a;
  ac = c - a;
  bc = c - b;
  float ab_sq = LengthSquared(ab);
  float ac_sq = LengthSquared(ac);
  float bc_sq = LengthSquared(bc);
  if (ab_sq < kEps || ac_sq < kEps || bc_sq < kEps) {
    return false;
  }
  Vec3F nn = Cross(c - b, a - b);
  float n_len = Length(nn);
  if (n_len < 1e-12f) {
    return false;
  }
  n = nn / n_len;
  plane_d = -Dot(n, a);
  ab_length = std::sqrt(ab_sq);
  ac_length = std::sqrt(ac_sq);
  bc_length = std::sqrt(bc_sq);
  ab_n = ab / ab_length;
  ac_n = ac / ac_length;
  bc_n = bc / bc_length;
  inv_ab_length_sq = 1.0f / ab_sq;
  inv_ac_length_sq = 1.0f / ac_sq;
  inv_bc_length_sq = 1.0f / bc_sq;
  ab_x_ac = Cross(ab, ac);
  bc_x_ba = Cross(bc, a - b);
  ac_x_ab = Cross(ac, b - a);

  if (ab_sq >= ac_sq && ab_sq >= bc_sq) {
    bound_center = a + ab * 0.5f;
    bound_radius_sq = ab_sq * 0.25f * 1.0022f;
    bound_radius = std::sqrt(bound_radius_sq);
    if (bound_radius_sq >= LengthSquared(bound_center - c)) {
      return true;
    }
  } else if (bc_sq >= ab_sq && bc_sq >= ac_sq) {
    bound_center = b + bc * 0.5f;
    bound_radius_sq = bc_sq * 0.25f * 1.0022f;
    bound_radius = std::sqrt(bound_radius_sq);
    if (bound_radius_sq >= LengthSquared(bound_center - a)) {
      return true;
    }
  } else {
    bound_center = a + ac * 0.5f;
    bound_radius_sq = ac_sq * 0.25f * 1.0022f;
    bound_radius = std::sqrt(bound_radius_sq);
    if (bound_radius_sq >= LengthSquared(bound_center - b)) {
      return true;
    }
  }

  Vec3F cd = ac * (-0.5f);
  Vec3F dh = Cross(n, ac);
  float den = 2.0f * Dot(bc, dh);
  if (std::fabs(den) < kEps) {
    return true;
  }
  float t = -(bc_sq + 2.0f * Dot(bc, cd)) / den;
  bound_center = c + cd + dh * t;
  bound_radius_sq = LengthSquared(bound_center - a) * 1.0022f;
  bound_radius = std::sqrt(bound_radius_sq);
  return true;
}

float CollisionTriangle::SignedDistance(const Vec3F &p) const {
  return Dot(n, p) + plane_d;
}

Vec3F CollisionTriangle::Project(const Vec3F &p) const {
  return p - n * (Dot(n, p) + plane_d);
}

bool CollisionTriangle::ContainsPoint(const Vec3F &p) const {
  if (Dot(Cross(ab, p - a), ab_x_ac) <= 0.0f) {
    return false;
  }
  if (Dot(Cross(bc, p - b), bc_x_ba) <= 0.0f) {
    return false;
  }
  if (Dot(Cross(ac, p - a), ac_x_ab) <= 0.0f) {
    return false;
  }
  return true;
}

Vec3F CollisionTriangle::ClosestPoint(const Vec3F &p) const {
  Vec3F ap = p - a;
  float d1 = Dot(ab, ap);
  float d2 = Dot(ac, ap);
  if (d1 <= 0.0f && d2 <= 0.0f) {
    return a;
  }
  Vec3F bp = p - b;
  float d3 = Dot(ab, bp);
  float d4 = Dot(ac, bp);
  if (d3 >= 0.0f && d4 <= d3) {
    return b;
  }
  float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
    float v = d1 / (d1 - d3);
    return a + ab * v;
  }
  Vec3F cp = p - c;
  float d5 = Dot(ab, cp);
  float d6 = Dot(ac, cp);
  if (d6 >= 0.0f && d5 <= d6) {
    return c;
  }
  float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
    float w = d2 / (d2 - d6);
    return a + ac * w;
  }
  float va = d3 * d6 - d5 * d4;
  if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
    float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return b + (c - b) * w;
  }
  float den = va + vb + vc;
  return a + ab * (vb / den) + ac * (vc / den);
}

bool SphereOverlapsTriangle(const MovingSphere &sph,
    const CollisionTriangle &tri) {
  float range = sph.radius + tri.bound_radius;
  if (LengthSquared(tri.bound_center - sph.center) > range * range) {
    return false;
  }
  Vec3F q = tri.ClosestPoint(sph.center);
  return LengthSquared(sph.center - q) <= sph.radius_sq;
}

SphereTriangleHit SphereTriangleContact(const MovingSphere &sph,
    const CollisionTriangle &tri) {
  SphereTriangleHit miss;
  float range = sph.radius + tri.bound_radius;
  if (LengthSquared(tri.bound_center - sph.center) > range * range) {
    return miss;
  }
  Vec3F q = tri.ClosestPoint(sph.center);
  Vec3F delta = sph.center - q;
  float d2 = LengthSquared(delta);
  if (d2 > sph.radius_sq) {
    return miss;
  }
  SphereTriangleHit hit;
  hit.hit = true;
  hit.time = 0.0f;
  hit.point = q;
  if (d2 > kEps) {
    hit.normal = delta;
  } else {
    hit.normal = tri.n;
  }
  return hit;
}

SphereTriangleHit SweptSphereVsTriangle(const MovingSphere &sph,
    const CollisionTriangle &tri) {
  SphereTriangleHit miss;
  float range = sph.velocity_length_plus_radius + tri.bound_radius;
  if (LengthSquared(sph.center - tri.bound_center) > range * range) {
    return miss;
  }
  if (sph.velocity_sq > kEps) {
    Vec3F perp = PerpToLine(tri.bound_center, sph.center, sph.velocity,
        sph.inv_velocity_sq);
    float rad = sph.radius + tri.bound_radius;
    if (LengthSquared(perp) > rad * rad) {
      return miss;
    }
  }

  SphereTriangleHit overlap = SphereTriangleContact(sph, tri);
  if (overlap.hit) {
    // A sphere whose skin is on the triangle is a contact, but it is not
    // buried. Treating that as t=0 swallows a parallel slide, so a craft
    // sitting on the mesh cannot move along the face. A move *into* the
    // plane is different: the face test is skipped while the center is in
    // the slab, the interior of a large face has no edges nearby, and the
    // sphere falls through. Freeze only when the velocity points through
    // the plane.
    if (sph.velocity_sq <= kEps) {
      return overlap;
    }
    float d2 = LengthSquared(sph.center - overlap.point);
    float inner = sph.radius * 0.997f;
    if (d2 <= inner * inner) {
      return overlap;
    }
    float sd = tri.SignedDistance(sph.center);
    float n_dot_v_skin = Dot(tri.n, sph.velocity);
    // kEps * |v| is smaller than float noise on a parallel slide, so a
    // remainder clipped onto the plane used to freeze at t=0 and pin the
    // craft to a crease. Require a real into-plane fraction of the move.
    if (sd * n_dot_v_skin < 0.0f &&
        std::fabs(n_dot_v_skin) > 1.0e-3f * sph.velocity_length) {
      return overlap;
    }
  }
  if (sph.velocity_sq <= kEps) {
    return miss;
  }

  float signed_dist = tri.SignedDistance(sph.center);
  float signed_dist_sq = signed_dist * signed_dist;
  bool in_slab = signed_dist_sq <= sph.radius_sq;
  float n_dot_v = Dot(tri.n, sph.velocity);

  if (std::fabs(n_dot_v) < kEps && !in_slab) {
    return miss;
  }

  // On a tilted face the orthogonal closest point can sit on an edge
  // outside the radius while the center's path still crosses the interior.
  // The face test then misses (ContainsPoint of the sit point is false),
  // in_slab skips it on the next frame, the edges are far, and the sphere
  // tunnels. If this move carries the center through the filled triangle,
  // hit when the skin meets the plane — even from outside the slab.
  if (signed_dist * n_dot_v < 0.0f &&
      std::fabs(n_dot_v) > kEps * sph.velocity_length) {
    float pierce_t = 1.0f;
    Vec3F pierce_p;
    if (LineSegmentPiercesTriangle(sph.center, sph.center + sph.velocity, tri,
        &pierce_t, &pierce_p)) {
      float t_skin = (std::fabs(signed_dist) - sph.radius) /
          std::fabs(n_dot_v);
      if (t_skin < 0.0f) {
        t_skin = 0.0f;
      }
      if (t_skin <= 1.0f) {
        SphereTriangleHit hit;
        hit.hit = true;
        hit.time = t_skin;
        hit.point = pierce_p;
        hit.normal = (signed_dist >= 0.0f) ? tri.n : tri.n * (-1.0f);
        return hit;
      }
    }
  }

  if (!in_slab) {
    Vec3F touch_offset;
    if (n_dot_v > 0.0f) {
      touch_offset = tri.n * sph.radius;
    } else {
      touch_offset = tri.n * (-sph.radius);
    }
    float t_center = -signed_dist / n_dot_v;
    float t_contact = t_center - sph.radius / std::fabs(n_dot_v);
    if (t_contact >= 0.0f) {
      if (t_contact > 1.0f) {
        return miss;
      }
      Vec3F p = sph.center + touch_offset + sph.velocity * t_contact;
      if (tri.ContainsPoint(p)) {
        SphereTriangleHit hit;
        hit.hit = true;
        hit.time = t_contact;
        hit.point = p;
        if (n_dot_v <= 0.0f) {
          hit.normal = tri.n;
        } else {
          hit.normal = tri.n * (-1.0f);
        }
        return hit;
      }
    }
  }

  SphereTriangleHit best;
  KeepEarlier(&best, HitPoint(sph, tri.a));
  KeepEarlier(&best, HitPoint(sph, tri.b));
  KeepEarlier(&best, HitPoint(sph, tri.c));
  KeepEarlier(&best, HitSegment(sph, tri.a, tri.ab_n, tri.ab_length));
  KeepEarlier(&best, HitSegment(sph, tri.a, tri.ac_n, tri.ac_length));
  KeepEarlier(&best, HitSegment(sph, tri.b, tri.bc_n, tri.bc_length));
  return best;
}

SphereTriangleHit SweptSphereVsTriangles(const MovingSphere &sph,
    const CollisionTriangle *tris, Si32 count) {
  SphereTriangleHit best;
  if (!tris || count <= 0) {
    return best;
  }
  for (Si32 i = 0; i < count; ++i) {
    KeepEarlier(&best, SweptSphereVsTriangle(sph, tris[i]));
  }
  return best;
}

bool LineSegmentPiercesTriangle(const Vec3F &p0, const Vec3F &p1,
    const CollisionTriangle &tri, float *out_t, Vec3F *out_p) {
  float d0 = tri.SignedDistance(p0);
  float d1 = tri.SignedDistance(p1);
  if (!((d0 > kEps && d1 < -kEps) || (d0 < -kEps && d1 > kEps))) {
    return false;
  }
  float denom = d0 - d1;
  if (std::fabs(denom) < kEps) {
    return false;
  }
  float t = d0 / denom;
  if (t <= 0.0f || t >= 1.0f) {
    return false;
  }
  Vec3F p = p0 + (p1 - p0) * t;
  Vec3F q = tri.ClosestPoint(p);
  float tol = tri.bound_radius * 1.0e-4f;
  if (tol < 1.0e-6f) {
    tol = 1.0e-6f;
  }
  if (LengthSquared(p - q) > tol * tol) {
    return false;
  }
  if (out_t) {
    *out_t = t;
  }
  if (out_p) {
    *out_p = p;
  }
  return true;
}

SpherePassThrough ClassifySpherePassThrough(const MovingSphere &sph,
    const CollisionTriangle &tri) {
  SpherePassThrough out;
  Vec3F end = sph.center + sph.velocity;
  MovingSphere at_start(sph.center, sph.radius, Vec3F(0.0f, 0.0f, 0.0f));
  MovingSphere at_end(end, sph.radius, Vec3F(0.0f, 0.0f, 0.0f));
  out.start_overlap = SphereOverlapsTriangle(at_start, tri);
  out.end_overlap = SphereOverlapsTriangle(at_end, tri);
  out.sweep = SweptSphereVsTriangle(sph, tri);
  out.center_pierce = LineSegmentPiercesTriangle(sph.center, end, tri,
      &out.pierce_t, &out.pierce_p);
  float d0 = tri.SignedDistance(sph.center);
  float d1 = tri.SignedDistance(end);
  bool toward_or_through = (d0 * d1 < 0.0f)
      || (std::fabs(d1) + 1.0e-4f < std::fabs(d0));
  out.entered_without_hit = !out.start_overlap && out.end_overlap
      && !out.sweep.hit && toward_or_through;
  out.passed = out.center_pierce || out.entered_without_hit;
  return out;
}

}  // namespace arctic
