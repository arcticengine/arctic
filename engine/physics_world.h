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

#ifndef ENGINE_PHYSICS_WORLD_H_
#define ENGINE_PHYSICS_WORLD_H_

#include <vector>

#include "engine/arctic_types.h"
#include "engine/collide_soup.h"
#include "engine/physics_contact.h"
#include "engine/physics_sphere_body.h"
#include "engine/physics_types.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// Facade over the static mesh and a set of sphere bodies. This is the
/// piece a game is expected to hold onto: build the mesh once, add a
/// sphere per craft, call SetWishVelocity and Step every frame, and read
/// Position/Contacts back afterward. Everything else in engine/physics_*
/// is reachable directly too, for callers (tests, diagnostics) that want
/// to work below this facade.
class PhysicsWorld {
 public:
  PhysicsWorld() {}

  void SetConfig(const PhysicsStepConfig &config) {
    config_ = config;
  }
  const PhysicsStepConfig &Config() const {
    return config_;
  }

  /// Builds (or replaces) the static collision mesh. See
  /// CollideSoup::Build for the parameters.
  void SetStaticMesh(const std::vector<Vec3F> &points_a,
      const std::vector<Vec3F> &points_b, const std::vector<Vec3F> &points_c,
      const std::vector<PhysicsMaterial> &materials,
      Si32 target_cells_per_axis = 256);
  const CollideSoup &Soup() const {
    return soup_;
  }

  /// Registers a new sphere body and returns its id. wish velocity starts
  /// at zero and the body's own push_weight (its share of any sphere vs.
  /// sphere separation, see SetPushWeight) starts at 0.5.
  PhysicsBodyId AddSphere(const Vec3F &position, float radius);
  void RemoveSphere(PhysicsBodyId id);
  bool IsAlive(PhysicsBodyId id) const;

  void SetPosition(PhysicsBodyId id, const Vec3F &position);
  void SetRadius(PhysicsBodyId id, float radius);
  void SetWishVelocity(PhysicsBodyId id, const Vec3F &wish_velocity);
  /// Sphere vs. sphere separation splits the overlap between the two
  /// bodies in proportion to their push_weight (a body with a smaller
  /// weight than the other one it overlaps moves less and makes the other
  /// move more, matching how Hover Racer let rivals give way to the player
  /// rather than splitting every push down the middle).
  void SetPushWeight(PhysicsBodyId id, float push_weight);

  /// Advances every live body by dt: each body substeps on its own
  /// (StepSphereBody, with its own persistent PhysicsManifold) against the
  /// static mesh, then every pair of bodies is separated as spheres in the
  /// same pass a mesh contact would have gone through -- there is no
  /// second whole-dt resolve afterward the way SeparateField needed one.
  void Step(float dt);

  Vec3F Position(PhysicsBodyId id) const;
  Vec3F Velocity(PhysicsBodyId id) const;
  /// What the last Step's residual-penetration cleanup added to position,
  /// zero when the last step found nothing to correct. See
  /// SphereStepResult::position_correction.
  Vec3F LastPositionCorrection(PhysicsBodyId id) const;
  const SphereBodySupport &Support(PhysicsBodyId id) const;
  const std::vector<ContactPoint> &Contacts(PhysicsBodyId id) const;
  ContactSolveOutcome Outcome(PhysicsBodyId id) const;

  /// Sweeps a sphere that is not one of the registered bodies (a camera
  /// boom, for instance) against the static mesh only.
  Vec3F SweepSphereQuery(const Vec3F &from, const Vec3F &to,
      float radius) const;
  bool HeightBelowQuery(float x, float z, float from_y, float max_drop,
      float radius, float *out_height) const;
  Vec3F DropSphereQuery(const Vec3F &start, float max_drop,
      float radius) const;
  /// Pushes center out of whatever it overlaps right now; useful to settle
  /// a body placed by teleport before the first Step.
  bool UnstickQuery(Vec3F *center, float radius) const;

 private:
  struct SphereBody {
    bool alive = false;
    Vec3F position = Vec3F(0.0f, 0.0f, 0.0f);
    float radius = 0.0f;
    Vec3F wish_velocity = Vec3F(0.0f, 0.0f, 0.0f);
    float push_weight = 0.5f;
    PhysicsManifold manifold;
    SphereStepResult last_result;
  };

  const SphereBody &BodyAt(PhysicsBodyId id) const;
  SphereBody &BodyAt(PhysicsBodyId id);
  void ResolveSphereSphere();

  CollideSoup soup_;
  PhysicsStepConfig config_;
  std::vector<SphereBody> bodies_;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_PHYSICS_WORLD_H_
