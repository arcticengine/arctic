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

#ifndef ENGINE_PHYSICS_SPHERE_BODY_H_
#define ENGINE_PHYSICS_SPHERE_BODY_H_

#include <vector>

#include "engine/arctic_types.h"
#include "engine/collide_soup.h"
#include "engine/physics_contact.h"
#include "engine/physics_solver.h"
#include "engine/physics_types.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// What a sphere's contact set says it is resting against, read back by the
/// game to drive things the engine has no opinion about (a hover spring
/// height, zeroing a downward lift once a floor is found, damping speed on
/// dirt). floor_normal is the flattest (largest n.y) floor face currently
/// touched; there is no equivalent "flattest wall" because Hover Racer only
/// ever needed to know a wall was there, not which one.
struct SphereBodySupport {
  bool has_floor = false;
  Vec3F floor_normal = Vec3F(0.0f, 1.0f, 0.0f);
  PhysicsMaterial floor_material;
  bool has_wall = false;
  bool has_ceiling = false;
};

/// Result of advancing a sphere body by one substep.
struct SphereStepResult {
  /// The velocity the solver actually allowed, i.e. wish_velocity clipped
  /// against every active contact. Position has already been advanced by
  /// velocity * dt by the time this is returned.
  Vec3F velocity = Vec3F(0.0f, 0.0f, 0.0f);
  Si32 contact_count = 0;
  ContactSolveOutcome outcome = ContactSolveOutcome::kFree;
  SphereBodySupport support;
  /// What the residual-penetration cleanup (the rate-limited push after the
  /// velocity solve) added to position this step, zero when it found
  /// nothing to correct. Exposed purely for diagnostics: a caller seeing
  /// this flip direction on consecutive steps is looking at the sphere
  /// being pushed back and forth between two opposing faces (e.g. a floor
  /// below and a low ceiling/ledge above) rather than converging.
  Vec3F position_correction = Vec3F(0.0f, 0.0f, 0.0f);
};

/// Narrow-phase for one sphere: finds every triangle within reach this
/// substep (the skin margin, plus however far wish_velocity could carry the
/// sphere in dt) and turns each into a ContactPoint with a signed
/// separation and the constraint_normal the solver should use for it (the
/// full geometric normal for a floor or ceiling, its horizontal projection
/// for a wall, so a wall can never absorb vertical motion). Exposed on its
/// own so callers that want to inspect contacts without integrating
/// anything (tests, diagnostics) do not have to go through StepSphereBody.
void GatherSphereContacts(const CollideSoup &soup, const Vec3F &center,
    float radius, const Vec3F &wish_velocity, float dt,
    const PhysicsStepConfig &config, std::vector<ContactPoint> *out_contacts,
    SphereBodySupport *out_support);

/// Advances *position by one substep of length dt under wish_velocity:
/// gathers contacts, updates manifold (pass nullptr for a one-off resolve
/// with no persistence across calls), solves for the largest velocity no
/// contact forbids, integrates, and finishes with a rate-limited position
/// correction for whatever penetration is still left afterward (a spawn
/// placement, a teleport, accumulated float error) so that correction never
/// shows up as an extra burst of velocity.
SphereStepResult StepSphereBody(const CollideSoup &soup,
    PhysicsManifold *manifold, Vec3F *position, float radius,
    const Vec3F &wish_velocity, float dt, const PhysicsStepConfig &config);

/// Sweeps a sphere from `from` to `to` and stops short of the first hit
/// (at 0.9 of the hit time, never exactly on the contact plane, so a sphere
/// sliding down a slope is not walked sideways off it by a snap to the
/// surface). No sliding, no contact resolution: this is for callers like a
/// camera boom that only ever want "as far as I can get without going
/// through a wall."
Vec3F SweepSphere(const CollideSoup &soup, const Vec3F &from,
    const Vec3F &to, float radius);

/// Pushes center directly out of whatever it already overlaps, without
/// otherwise moving it, iterating on the shallowest overlap first. Returns
/// false (and reverts to the starting position) if it could not converge
/// within max_iterations. A contact whose push would be downward through a
/// floor-like face (n.y > 0.35, whether or not that reaches the
/// floor/ceiling/wall threshold) is skipped, so unsticking never drops the
/// sphere through the ground it is resting on.
bool UnstickSphereBody(const CollideSoup &soup, Vec3F *center, float radius,
    Si32 max_iterations = 20);

/// If the open segment from-to would carry the center through a triangle,
/// stops at 0.9 of the first such pierce. A safety net for when a swept
/// test missed a face the straight-line center path still crosses.
Vec3F ClampSegmentToSurface(const CollideSoup &soup, const Vec3F &from,
    const Vec3F &to);

/// Sweeps straight down from (x, from_y, z) by at most max_drop and reports
/// the surface height under it: the radius has already been taken off the
/// center the sphere would touch down at, so a body resting there has its
/// center at *out_height + radius. Only floor-facing triangles
/// (n.y >= floor_normal_y) count, so a steep face reports nothing rather
/// than a height nothing can rest on. Returns false when no floor was
/// found within max_drop, leaving *out_height untouched. This is a
/// measurement, not a place to put a body, so it reports the height of the
/// touch itself and keeps no safety margin off it.
bool QuerySphereHeightBelow(const CollideSoup &soup, float x, float z,
    float from_y, float max_drop, float radius, float floor_normal_y,
    float *out_height);

/// Drops center straight down by at most max_drop until its skin rests on
/// the mesh: sweep, clamp to the surface, unstick, clamp again. With
/// nothing below, the whole max_drop is taken, so telling a landing from a
/// miss means comparing the fall against max_drop rather than looking for
/// the start position to come back. The start position comes back when the
/// drop could not begin at all, which a near-vertical face grazing the
/// fall path is enough to cause: the swept test reports it as a hit at
/// time zero (see SweepSphere). A returned start therefore means "could
/// not move", not "nothing below", and this is not a way to measure the
/// height of a surface running alongside such a face.
Vec3F DropSphereBody(const CollideSoup &soup, const Vec3F &start,
    float max_drop, float radius);

/// @}

}  // namespace arctic

#endif  // ENGINE_PHYSICS_SPHERE_BODY_H_
