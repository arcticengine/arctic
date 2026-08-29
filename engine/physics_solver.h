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

#ifndef ENGINE_PHYSICS_SOLVER_H_
#define ENGINE_PHYSICS_SOLVER_H_

#include "engine/arctic_types.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// One half-space velocity constraint: the solver only ever enforces
/// Dot(normal, v) <= bound. normal points INTO the surface being
/// constrained, i.e. it is the direction whose closing speed is being
/// limited -- the opposite sign from ContactPoint::normal and from
/// SphereTriangleHit::normal, which both point from the surface toward the
/// sphere. A plain contact sets bound = max(0, separation / dt): while
/// separation is positive the surface may still be approached this step,
/// just not closed faster than it takes to reach zero separation; once
/// separation has gone negative (an existing overlap) the bound clamps to
/// 0 rather than going negative, so the velocity solve only ever stops the
/// approach and never manufactures an outward velocity to fix a
/// penetration that already exists -- that correction is a position-only
/// pass, on purpose, so it cannot inject energy into the velocity. A
/// contact that must not eat perpendicular motion (Hover Racer's walls
/// must never rob the hover lift) simply has normal's Y component zeroed
/// out and renormalized before it reaches the solver.
struct SpeculativeConstraint {
  Vec3F normal = Vec3F(0.0f, 1.0f, 0.0f);
  float bound = 0.0f;
};

/// Which case the exact block solver landed in, or that it fell back to
/// PGS. kDegenerate means the active constraints have no common feasible
/// tangent at all (an inward-facing wedge with no way out); the result is
/// then a clean stop rather than a direction picked by whichever plane a
/// sequential clip happened to visit last.
enum class ContactSolveOutcome : Ui8 {
  kFree = 0,
  kFace = 1,
  kEdge = 2,
  kCorner = 3,
  kDegenerate = 4,
  kPgs = 5,
};

struct ContactSolveResult {
  Vec3F velocity = Vec3F(0.0f, 0.0f, 0.0f);
  /// Bit i set means constraints[i] is exactly active (holding its bound)
  /// in the result. Only meaningful up to 32 constraints; kPgs results set
  /// a bit for every constraint the relaxation ever pushed against.
  Ui32 active_mask = 0;
  ContactSolveOutcome outcome = ContactSolveOutcome::kFree;
};

/// Projects wish_velocity onto the intersection of the half-spaces
/// Dot(constraints[i].normal, v) <= constraints[i].bound, i.e. finds the
/// velocity closest to wish_velocity (in the ordinary Euclidean sense) that
/// violates none of them.
///
/// count <= 3 is solved exactly by enumerating which subset of constraints
/// is active and solving the resulting 1x1, 2x2 or 3x3 linear system for
/// the Lagrange multipliers (the "block solve"); this is what tells a
/// corner pinched between three faces apart from an edge shared by two,
/// instead of sequential clipping producing a different, sometimes
/// opposite, answer depending which face happened to be visited last.
///
/// count > 3 falls back to Projected Gauss-Seidel relaxation over
/// constraints in the order given (callers should pass them in a
/// canonical order, e.g. PhysicsManifold's tri_index/kind/sub order, so the
/// fallback is deterministic too).
ContactSolveResult SolveContactVelocity(const Vec3F &wish_velocity,
    const SpeculativeConstraint *constraints, Si32 count);

/// @}

}  // namespace arctic

#endif  // ENGINE_PHYSICS_SOLVER_H_
