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

#include "engine/physics_sphere_body.h"

#include <algorithm>
#include <cmath>

namespace arctic {

namespace {

const float kEps = 1.0e-8f;

Vec3F AwayDirection(const CollisionTriangle &tri, const Vec3F &center,
    const Vec3F &closest, float *out_dist) {
  Vec3F delta = center - closest;
  float dist = Length(delta);
  *out_dist = dist;
  if (dist > kEps) {
    return delta / dist;
  }
  return NormalizeSafe(tri.n);
}

}  // namespace

void GatherSphereContacts(const CollideSoup &soup, const Vec3F &center,
    float radius, const Vec3F &wish_velocity, float dt,
    const PhysicsStepConfig &config, std::vector<ContactPoint> *out_contacts,
    SphereBodySupport *out_support) {
  out_contacts->clear();
  SphereBodySupport support;
  float travel = Length(wish_velocity) * std::max(dt, 0.0f);
  float reach = config.skin + travel;
  Vec3F end = center + wish_velocity * dt;
  float best_floor_ny = -2.0f;

  // The "standing over a lip" fallback below probes a full three radii
  // straight down for a floor, on the theory that a rolling sphere should
  // not lose its footing over a gap that is small next to its own size.
  // The broad-phase query has to be widened to match, or a floor past
  // (radius + reach) is never even offered to that fallback in the first
  // place -- exactly what let a fast, steep descent (see
  // test_physics_sphere_body_reacquires_steep_floor_gap) go from a solid
  // floor contact to none at all for the rest of the run instead of
  // catching the next facet a bit further down.
  float floor_probe = radius * 3.0f;
  float query_reach = std::max(radius + reach, floor_probe);
  soup.ForEachNearSegment(center, end, query_reach, [&](Si32 idx) {
    const CollisionTriangle &tri = soup.Triangle(idx);
    Vec3F closest = tri.ClosestPoint(center);
    float dist = 0.0f;
    Vec3F away = AwayDirection(tri, center, closest, &dist);
    float separation = dist - radius;
    bool close_enough = separation <= reach;
    Vec3F point = closest;
    // A near-flat floor's true closest point can land on a far edge while a
    // point straight below the center is still solidly inside the face
    // (a tilted triangle's 3D nearest point is not always the same as the
    // point a straight vertical probe would land on). Catch that "standing
    // over a lip" case the same way Hover Racer's old FindRideSupport did,
    // by trying a straight vertical pierce -- gated on the same distance
    // the pierce itself travels, not the much tighter contact reach, or
    // the pierce below is dead code past (radius + reach).
    if (!close_enough && tri.n.y >= config.floor_normal_y) {
      float plane_d = tri.SignedDistance(center);
      if (std::fabs(plane_d) <= floor_probe) {
        Vec3F below = center;
        below.y -= floor_probe;
        float t = 1.0f;
        Vec3F p;
        if (LineSegmentPiercesTriangle(center, below, tri, &t, &p)) {
          close_enough = true;
          away = tri.n;
          separation = plane_d - radius;
          point = p;
        }
      }
    }
    if (!close_enough) {
      return;
    }
    ContactPoint c;
    c.feature = ClassifyTriangleFeature(tri, center, idx);
    c.point = point;
    c.normal = away;
    c.separation = separation;
    c.material = soup.Material(idx);
    // Support means the sphere is resting on this face, which is a
    // stricter thing than having a constraint against it. A contact is
    // made for anything within the speculative reach, and the vertical
    // pierce above deliberately reaches a full three radii down so a
    // rolling sphere cannot lose a floor over a gap that is small next to
    // its own size -- but a face most of two radii away is one the sphere
    // is flying over, not standing on. Callers use has_floor to stop
    // pushing a body down (see SphereBodySupport), so claiming support
    // that far out deadlocks them: they will not descend because they are
    // told they have landed, and they never land because they do not
    // descend. Only a face inside the contact skin counts.
    bool touching = separation <= config.skin;
    float ny = tri.n.y;
    if (ny >= config.floor_normal_y) {
      c.constraint_normal = away;
      if (touching) {
        support.has_floor = true;
        if (ny > best_floor_ny) {
          best_floor_ny = ny;
          support.floor_normal = tri.n;
          support.floor_material = c.material;
        }
      }
    } else if (ny <= -config.floor_normal_y) {
      c.constraint_normal = away;
      support.has_ceiling = true;
    } else if (away.y >= config.floor_normal_y) {
      // The sphere is riding over the top of this face, not running into
      // its side. Flattening the contact into the XZ plane (below) is what
      // keeps a body sliding along a wall instead of being launched up it,
      // but it is only meaningful while the wall is actually beside the
      // sphere. A seam between two road tiles hangs a skirt straight down
      // from the surface, so its topmost edge sits at exactly the level
      // the sphere rolls on: flattened, that level seam becomes a vertical
      // wall as tall as the whole skirt, pointing straight back along the
      // ride, and the sphere stops dead in the middle of the road (see
      // test_physics_sphere_body_rolls_over_road_seam_skirt). The
      // direction out of the feature is nearly straight up here, which
      // costs the ride nothing, so keep it whole. Support is deliberately
      // left alone: resting on an edge is not resting on a floor face,
      // and the floor the sphere rides on reports itself.
      c.constraint_normal = away;
    } else {
      Vec3F away_xz(away.x, 0.0f, away.z);
      float xl = Length(away_xz);
      if (xl < 1.0e-5f) {
        return;
      }
      c.constraint_normal = away_xz / xl;
      support.has_wall = true;
    }
    out_contacts->push_back(c);
  });
  if (out_support) {
    *out_support = support;
  }
}

namespace {

// How alarming a solver outcome is, so a merged report can keep the worst
// one. A degenerate wedge (no feasible tangent at all) matters most; the
// PGS fallback is next because it means the exact solver did not apply.
Si32 OutcomeSeverity(ContactSolveOutcome outcome) {
  switch (outcome) {
    case ContactSolveOutcome::kFree:
      return 0;
    case ContactSolveOutcome::kFace:
      return 1;
    case ContactSolveOutcome::kEdge:
      return 2;
    case ContactSolveOutcome::kCorner:
      return 3;
    case ContactSolveOutcome::kPgs:
      return 4;
    case ContactSolveOutcome::kDegenerate:
      return 5;
  }
  return 0;
}

}  // namespace

SphereStepResult MergeSphereStepResults(const SphereStepResult &earlier,
    const SphereStepResult &later) {
  SphereStepResult out = later;
  out.contact_count = std::max(earlier.contact_count, later.contact_count);
  if (OutcomeSeverity(earlier.outcome) > OutcomeSeverity(later.outcome)) {
    out.outcome = earlier.outcome;
  }
  out.position_correction = earlier.position_correction
      + later.position_correction;
  out.substep_budget_exhausted = earlier.substep_budget_exhausted
      || later.substep_budget_exhausted;
  out.support.has_wall = earlier.support.has_wall || later.support.has_wall;
  out.support.has_ceiling = earlier.support.has_ceiling
      || later.support.has_ceiling;
  if (earlier.support.has_floor && !later.support.has_floor) {
    out.support.has_floor = true;
    out.support.floor_normal = earlier.support.floor_normal;
    out.support.floor_material = earlier.support.floor_material;
  } else if (earlier.support.has_floor && later.support.has_floor
      && earlier.support.floor_normal.y > later.support.floor_normal.y) {
    // Same rule as within one substep: the flattest floor touched wins, so
    // a game reading the floor normal gets the face it is actually riding
    // and not whichever steep facet happened to be touched last.
    out.support.floor_normal = earlier.support.floor_normal;
    out.support.floor_material = earlier.support.floor_material;
  }
  return out;
}

SphereStepResult StepSphereBody(const CollideSoup &soup,
    PhysicsManifold *manifold, Vec3F *position, float radius,
    const Vec3F &wish_velocity, float dt, const PhysicsStepConfig &config) {
  SphereStepResult result;
  if (dt <= 0.0f) {
    result.velocity = wish_velocity;
    return result;
  }

  std::vector<ContactPoint> fresh;
  GatherSphereContacts(soup, *position, radius, wish_velocity, dt, config,
      &fresh, &result.support);

  const std::vector<ContactPoint> *contacts = &fresh;
  if (manifold) {
    manifold->Update(&fresh);
    contacts = &manifold->Contacts();
  } else {
    std::sort(fresh.begin(), fresh.end(),
        [](const ContactPoint &a, const ContactPoint &b) {
          if (a.feature.tri_index != b.feature.tri_index) {
            return a.feature.tri_index < b.feature.tri_index;
          }
          if (a.feature.kind != b.feature.kind) {
            return a.feature.kind < b.feature.kind;
          }
          return a.feature.sub < b.feature.sub;
        });
  }

  std::vector<SpeculativeConstraint> cs;
  cs.reserve(contacts->size());
  for (const ContactPoint &c : *contacts) {
    SpeculativeConstraint sc;
    sc.normal = c.constraint_normal * -1.0f;
    sc.bound = std::max(0.0f, c.separation / dt);
    cs.push_back(sc);
  }

  ContactSolveResult solved = SolveContactVelocity(wish_velocity, cs.data(),
      static_cast<Si32>(cs.size()));
  result.velocity = solved.velocity;
  result.outcome = solved.outcome;
  result.contact_count = static_cast<Si32>(cs.size());

  // Stamp the realized closing speed back onto the persisted contacts so
  // the next frame's Update() carries it forward as warm_speed for whatever
  // still matches by feature id. The exact block solver does not need this
  // to produce a correct answer, but the PGS fallback and any future
  // iterative solver can seed their first guess from it.
  if (manifold) {
    std::vector<ContactPoint> &mutable_contacts = manifold->MutableContacts();
    for (Si32 i = 0; i < static_cast<Si32>(cs.size())
        && i < static_cast<Si32>(mutable_contacts.size()); ++i) {
      mutable_contacts[i].warm_speed = Dot(cs[i].normal, solved.velocity);
    }
  }

  *position += solved.velocity * dt;

  // Whether a floor face is in play at all, which is looser than support:
  // support means the sphere is already resting on one (see
  // GatherSphereContacts), while the seam case the assist further down
  // exists for is precisely a sphere a hair above the floor, which on a
  // fast-moving body can be further out than the contact skin.
  bool floor_contact_near = false;
  for (const ContactPoint &c : *contacts) {
    Si32 idx = c.feature.tri_index;
    if (idx < 0 || idx >= soup.TriangleCount()) {
      continue;
    }
    if (soup.Triangle(idx).n.y >= config.floor_normal_y) {
      floor_contact_near = true;
      break;
    }
  }

  // Residual-penetration cleanup: a single rate-limited push along whatever
  // is deepest right now, never a full snap. The velocity solve above
  // already keeps this step from creating new penetration on its own, so
  // this only ever has real work to do after a teleport, a spawn, or float
  // slop -- which is exactly why it must not add velocity.
  float best_pen = 0.0f;
  Vec3F best_dir(0.0f, 1.0f, 0.0f);
  bool found_pen = false;
  soup.ForEachNear(
      Bound3F(position->x - radius, position->x + radius,
          position->y - radius, position->y + radius,
          position->z - radius, position->z + radius),
      [&](Si32 idx) {
        const CollisionTriangle &tri = soup.Triangle(idx);
        Vec3F closest = tri.ClosestPoint(*position);
        float dist = 0.0f;
        Vec3F away = AwayDirection(tri, *position, closest, &dist);
        Vec3F delta = *position - closest;
        if (delta.y < 0.0f && tri.n.y > 0.35f) {
          return;
        }
        float pen = radius - dist;
        if (pen > best_pen) {
          best_pen = pen;
          best_dir = away;
          found_pen = true;
        }
      });
  if (found_pen && best_pen > 0.0f) {
    float correction = std::min(best_pen * config.position_correction_rate,
        config.max_position_correction);
    result.position_correction = best_dir * correction;
    *position += result.position_correction;
  } else if (floor_contact_near && !result.support.has_wall
      && !result.support.has_ceiling) {
    // Surface-conforming assist: rolling across the seam between two
    // facets of a faceted (low-poly) floor, the sphere is briefly a hair
    // above both of them -- no penetration for the block above to correct,
    // but nothing pulls it back down either, so with a purely horizontal
    // wish the velocity solve finds every nearby facet already satisfied
    // and returns however much of the wish was already free (see
    // GatherSphereContacts/SolveContactVelocity), which is often close to
    // no vertical motion at all. Repeated every few facets, that reads as
    // the ride stuttering to a dead stop and lurching forward again in
    // sync with the mesh's own triangle seams. Nudging the sphere down
    // onto the closest floor facet still within the contact skin, at the
    // same never-more-than-a-nudge rate as the penetration case above,
    // keeps it in contact through the seam instead of coasting free.
    // Same reach GatherSphereContacts used to decide a facet counts as a
    // contact at all, so nothing this step already treated as a contact
    // is out of reach for closing.
    float reach = config.skin + Length(wish_velocity) * dt;
    float best_gap = reach;
    Vec3F gap_dir(0.0f, 1.0f, 0.0f);
    bool found_gap = false;
    // The box has to reach past the sphere's own surface by that same
    // margin, not stop at it: a facet that is merely close (a positive
    // gap, by definition outside the sphere) sits just past that surface,
    // and a box that stopped exactly at the surface would never overlap a
    // floor bin sitting only a hair below it.
    soup.ForEachNear(
        Bound3F(position->x - radius - reach, position->x + radius + reach,
            position->y - radius - reach, position->y + radius + reach,
            position->z - radius - reach, position->z + radius + reach),
        [&](Si32 idx) {
          const CollisionTriangle &tri = soup.Triangle(idx);
          if (tri.n.y < config.floor_normal_y) {
            return;
          }
          Vec3F closest = tri.ClosestPoint(*position);
          float dist = 0.0f;
          Vec3F away = AwayDirection(tri, *position, closest, &dist);
          float gap = dist - radius;
          if (gap > 0.0f && gap < best_gap) {
            best_gap = gap;
            gap_dir = away;
            found_gap = true;
          }
        });
    if (found_gap) {
      float correction = std::min(best_gap * config.position_correction_rate,
          config.max_position_correction);
      result.position_correction = gap_dir * -correction;
      *position += result.position_correction;
    }
  }

  MeasureSphereFloorBelow(soup, *position, radius, config, &result.support);

  return result;
}

namespace {

// Turns a margin in world units into a fraction of a path of this length,
// so the sweeps below can stop a fixed distance short of what they hit.
float BackoffFraction(float travel, float backoff) {
  if (travel <= 1.0e-10f || backoff <= 0.0f) {
    return 0.0f;
  }
  return backoff / travel;
}

}  // namespace

SweepSphereResult SweepSphere(const CollideSoup &soup, const Vec3F &from,
    const Vec3F &to, float radius, float backoff) {
  SweepSphereResult out;
  out.position = to;
  Vec3F vel = to - from;
  float travel = Length(vel);
  if (travel <= 1.0e-5f) {
    return out;
  }
  MovingSphere sph(from, radius, vel);
  SphereTriangleHit best;
  soup.ForEachNearSegment(from, to, radius, [&](Si32 idx) {
    SphereTriangleHit hit = SweptSphereVsTriangle(sph, soup.Triangle(idx));
    if (hit.hit && (!best.hit || hit.time < best.time)) {
      best = hit;
    }
  });
  if (!best.hit || best.time >= 1.0f) {
    return out;
  }
  out.normal = NormalizeSafe(best.normal);
  out.time = best.time;
  float t = best.time - BackoffFraction(travel, backoff);
  if (t <= 0.0f) {
    t = 0.0f;
    out.kind = (best.time <= 0.0f) ? SweepSphereResult::Kind::kStartOverlap
        : SweepSphereResult::Kind::kHit;
  } else {
    out.kind = SweepSphereResult::Kind::kHit;
  }
  out.position = from + vel * t;
  return out;
}

bool UnstickSphereBody(const CollideSoup &soup, Vec3F *center, float radius,
    Si32 max_iterations) {
  Vec3F start = *center;
  for (Si32 step = 0; step < max_iterations; ++step) {
    MovingSphere sph(*center, radius, Vec3F(0.0f, 0.0f, 0.0f));
    SphereTriangleHit best;
    float best_d2 = -1.0f;
    Bound3F box(center->x - radius, center->x + radius,
        center->y - radius, center->y + radius,
        center->z - radius, center->z + radius);
    soup.ForEachNear(box, [&](Si32 idx) {
      const CollisionTriangle &tri = soup.Triangle(idx);
      SphereTriangleHit hit = SphereTriangleContact(sph, tri);
      if (!hit.hit) {
        return;
      }
      Vec3F delta = *center - hit.point;
      if (delta.y < 0.0f && tri.n.y > 0.35f) {
        return;
      }
      float d2 = LengthSquared(delta);
      float inner = radius * 0.997f;
      if (d2 >= inner * inner) {
        return;
      }
      if (d2 > best_d2) {
        best_d2 = d2;
        best = hit;
      }
    });
    if (!best.hit) {
      return true;
    }
    Vec3F delta = *center - best.point;
    float dist = Length(delta);
    if (dist < 1.0e-8f) {
      delta = Vec3F(0.0f, 1.0f, 0.0f);
      dist = 1.0f;
    }
    float gap = radius - dist;
    if (gap <= 0.0f) {
      return true;
    }
    *center = *center + (delta / dist) * gap;
  }
  *center = start;
  return false;
}

Vec3F ClampSegmentToSurface(const CollideSoup &soup, const Vec3F &from,
    const Vec3F &to, float backoff) {
  float travel = Length(to - from);
  if (travel <= 1.0e-10f) {
    return to;
  }
  float best_t = 1.0f;
  soup.ForEachNearSegment(from, to, 0.0f, [&](Si32 idx) {
    float t = 1.0f;
    Vec3F p;
    if (!LineSegmentPiercesTriangle(from, to, soup.Triangle(idx), &t, &p)) {
      return;
    }
    if (t > 0.0f && t < best_t) {
      best_t = t;
    }
  });
  if (best_t >= 1.0f) {
    return to;
  }
  float s = std::max(0.0f, best_t - BackoffFraction(travel, backoff));
  return from + (to - from) * s;
}

bool QuerySphereHeightBelow(const CollideSoup &soup, float x, float z,
    float from_y, float max_drop, float radius, float floor_normal_y,
    float *out_height) {
  if (max_drop <= 1.0e-4f) {
    *out_height = from_y - radius;
    return true;
  }
  Vec3F start(x, from_y, z);
  Vec3F vel(0.0f, -max_drop, 0.0f);
  Vec3F end = start + vel;
  MovingSphere sph(start, radius, vel);
  SphereTriangleHit best;
  float best_pierce = 1.0f;
  bool any_pierce = false;
  soup.ForEachNearSegment(start, end, radius, [&](Si32 idx) {
    const CollisionTriangle &tri = soup.Triangle(idx);
    if (tri.n.y < floor_normal_y) {
      return;
    }
    SphereTriangleHit hit = SweptSphereVsTriangle(sph, tri);
    if (hit.hit && (!best.hit || hit.time < best.time)) {
      best = hit;
    }
    float t = 1.0f;
    Vec3F p;
    if (LineSegmentPiercesTriangle(start, end, tri, &t, &p)) {
      if (t > 0.0f && (!any_pierce || t < best_pierce)) {
        best_pierce = t;
        any_pierce = true;
      }
    }
  });
  // No safety back-off from the moment of touch here, unlike the sweeps
  // that hand back a place to put a body: nothing is ever placed at the
  // height this reports, so shaving the last tenth of the fall off would
  // only bias the answer by a tenth of however far the probe happened to
  // fall -- a hover control reading this would sit that much low, and the
  // further the probe reaches, the worse it reads. The same goes for the
  // end of the range: a floor exactly max_drop below is a floor found, and
  // its height is worth reporting.
  if (best.hit && best.time <= 1.0f) {
    *out_height = (start + vel * best.time).y - radius;
    return true;
  }
  if (any_pierce) {
    // The pierce is where the center line crosses the face, so that point
    // is already on the surface -- no radius comes off it.
    *out_height = (start + vel * best_pierce).y;
    return true;
  }
  return false;
}

SphereDropResult DropSphereBody(const CollideSoup &soup, const Vec3F &start,
    float max_drop, float radius, float backoff) {
  SphereDropResult result;
  result.position = start;
  result.kind = SphereDropResult::Kind::kBlockedAtStart;
  if (max_drop <= 1.0e-4f) {
    return result;
  }
  Vec3F dest = start;
  dest.y -= max_drop;
  SweepSphereResult sweep = SweepSphere(soup, start, dest, radius, backoff);
  bool sweep_hit = sweep.kind != SweepSphereResult::Kind::kClear;
  Vec3F at = ClampSegmentToSurface(soup, start, sweep.position, backoff);
  UnstickSphereBody(soup, &at, radius);
  at = ClampSegmentToSurface(soup, start, at, backoff);
  result.position = at;
  result.fall = start.y - at.y;
  if (sweep_hit) {
    result.normal = sweep.normal;
  }
  float slack = std::max(1.0e-6f, max_drop * 1.0e-5f);
  if (result.fall <= slack) {
    result.fall = 0.0f;
    result.position = start;
    result.kind = SphereDropResult::Kind::kBlockedAtStart;
  } else if (!sweep_hit && result.fall >= max_drop - slack) {
    result.kind = SphereDropResult::Kind::kNothingBelow;
    result.normal = Vec3F(0.0f, 0.0f, 0.0f);
  } else {
    result.kind = SphereDropResult::Kind::kLanded;
  }
  return result;
}

void MeasureSphereFloorBelow(const CollideSoup &soup, const Vec3F &center,
    float radius, const PhysicsStepConfig &config,
    SphereBodySupport *support) {
  if (support == nullptr) {
    return;
  }
  support->has_floor_below = false;
  support->floor_distance = 0.0f;
  if (config.support_probe <= 0.0f) {
    return;
  }
  // The sweep starts a skin above the center so a body resting inside the
  // contact skin is not measured from inside the surface it stands on: a
  // sweep that begins already touching reports the touch at time zero, and
  // the reading would then saturate at whatever the lift-off was. A skin is
  // the smallest lift that clears legitimate penetration, so the worst a
  // sideways graze can now cost is one skin of made-up depth instead of the
  // half unit Hover Racer's own probe used to invent.
  float lift = config.skin;
  float surface = 0.0f;
  if (!QuerySphereHeightBelow(soup, center.x, center.z, center.y + lift,
      config.support_probe + lift, radius, config.floor_normal_y,
      &surface)) {
    return;
  }
  support->has_floor_below = true;
  support->floor_distance = center.y - (surface + radius);
}

}  // namespace arctic
