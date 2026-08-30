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

#include "engine/physics_world.h"

#include <algorithm>
#include <cmath>

namespace arctic {

namespace {

const std::vector<ContactPoint> kInvalidContacts;

}  // namespace

void PhysicsWorld::SetStaticMesh(const std::vector<Vec3F> &points_a,
    const std::vector<Vec3F> &points_b, const std::vector<Vec3F> &points_c,
    const std::vector<PhysicsMaterial> &materials,
    Si32 target_cells_per_axis) {
  soup_.Build(points_a, points_b, points_c, materials, target_cells_per_axis);
}

PhysicsBodyId PhysicsWorld::AddSphere(const Vec3F &position, float radius) {
  for (size_t i = 0; i < bodies_.size(); ++i) {
    if (!bodies_[i].alive) {
      bodies_[i] = SphereBody();
      bodies_[i].alive = true;
      bodies_[i].position = position;
      bodies_[i].radius = radius;
      return static_cast<PhysicsBodyId>(i);
    }
  }
  SphereBody body;
  body.alive = true;
  body.position = position;
  body.radius = radius;
  bodies_.push_back(body);
  return static_cast<PhysicsBodyId>(bodies_.size() - 1);
}

void PhysicsWorld::RemoveSphere(PhysicsBodyId id) {
  if (id < 0 || static_cast<size_t>(id) >= bodies_.size()) {
    return;
  }
  bodies_[static_cast<size_t>(id)] = SphereBody();
}

bool PhysicsWorld::IsAlive(PhysicsBodyId id) const {
  if (id < 0 || static_cast<size_t>(id) >= bodies_.size()) {
    return false;
  }
  return bodies_[static_cast<size_t>(id)].alive;
}

const PhysicsWorld::SphereBody &PhysicsWorld::BodyAt(PhysicsBodyId id) const {
  static const SphereBody kInvalidBody;
  if (id < 0 || static_cast<size_t>(id) >= bodies_.size() ||
      !bodies_[static_cast<size_t>(id)].alive) {
    return kInvalidBody;
  }
  return bodies_[static_cast<size_t>(id)];
}

PhysicsWorld::SphereBody &PhysicsWorld::BodyAt(PhysicsBodyId id) {
  static SphereBody kDummyBody;
  if (id < 0 || static_cast<size_t>(id) >= bodies_.size() ||
      !bodies_[static_cast<size_t>(id)].alive) {
    kDummyBody = SphereBody();
    return kDummyBody;
  }
  return bodies_[static_cast<size_t>(id)];
}

void PhysicsWorld::Teleport(PhysicsBodyId id, const Vec3F &position) {
  if (!IsAlive(id)) {
    return;
  }
  SphereBody &body = BodyAt(id);
  body.position = position;
  MeasureSphereFloorBelow(soup_, body.position, body.radius, config_,
      &body.last_result.support);
  ++teleports_since_step_;
}

void PhysicsWorld::SetRadius(PhysicsBodyId id, float radius) {
  BodyAt(id).radius = radius;
}

void PhysicsWorld::SetWishVelocity(PhysicsBodyId id,
    const Vec3F &wish_velocity) {
  BodyAt(id).wish_velocity = wish_velocity;
}

void PhysicsWorld::SetPushWeight(PhysicsBodyId id, float push_weight) {
  BodyAt(id).push_weight = push_weight;
}

Vec3F PhysicsWorld::PushBodyAside(const SphereBody &body,
    const Vec3F &shift) const {
  SweepSphereResult swept = SweepSphere(soup_, body.position,
      body.position + shift, body.radius, config_.skin);
  Vec3F at = swept.position;
  UnstickSphereBody(soup_, &at, body.radius);
  return at;
}

void PhysicsWorld::ResolveSphereSphere() {
  for (size_t i = 0; i < bodies_.size(); ++i) {
    if (!bodies_[i].alive) {
      continue;
    }
    for (size_t j = i + 1; j < bodies_.size(); ++j) {
      if (!bodies_[j].alive) {
        continue;
      }
      SphereBody &a = bodies_[i];
      SphereBody &b = bodies_[j];
      Vec3F d = a.position - b.position;
      d.y = 0.0f;
      float dist = Length(d);
      float min_d = (a.radius + b.radius) * 1.02f;
      Vec3F n(1.0f, 0.0f, 0.0f);
      float push = 0.0f;
      if (dist < 1.0e-4f) {
        push = min_d * 0.5f;
      } else if (dist < min_d) {
        push = min_d - dist;
        n = d / dist;
      } else {
        continue;
      }
      float wsum = a.push_weight + b.push_weight;
      float wa = (wsum > 1.0e-6f) ? a.push_weight / wsum : 0.5f;
      float wb = (wsum > 1.0e-6f) ? b.push_weight / wsum : 0.5f;
      // The push is swept, not written: a body pressed against a wall by
      // the one beside it would otherwise be moved straight into the mesh
      // and left for the next step's push-out to dig back out, which is the
      // same "write the position and hope" the solver exists to avoid. Only
      // X and Z move, as before -- separating racers must not lift them.
      a.position = PushBodyAside(a, Vec3F(n.x * push * wa, 0.0f,
          n.z * push * wa));
      b.position = PushBodyAside(b, Vec3F(-n.x * push * wb, 0.0f,
          -n.z * push * wb));
    }
  }
}

void PhysicsWorld::Step(float dt) {
  if (dt <= 0.0f) {
    return;
  }
  teleports_since_step_ = 0;
  for (SphereBody &body : bodies_) {
    if (!body.alive) {
      continue;
    }
    float move = Length(body.wish_velocity) * dt;
    float max_move = std::max(config_.max_substep_move, 1.0e-4f);
    Si32 wanted = 1 + static_cast<Si32>(move / max_move);
    Si32 steps = std::min(wanted, std::max(config_.max_substeps, 1));
    steps = std::max(steps, 1);
    float sdt = dt / static_cast<float>(steps);
    SphereStepResult report;
    for (Si32 s = 0; s < steps; ++s) {
      SphereStepResult substep = StepSphereBody(soup_, &body.manifold,
          &body.position, body.radius, body.wish_velocity, sdt, config_);
      report = (s == 0) ? substep : MergeSphereStepResults(report, substep);
    }
    // A step that wanted more substeps than the cap allows moved further per
    // substep than max_substep_move, which is the anti-tunneling promise;
    // say so in the report rather than letting it pass in silence.
    report.substep_budget_exhausted = wanted > steps;
    body.last_result = report;
  }
  ResolveSphereSphere();
}

Vec3F PhysicsWorld::Position(PhysicsBodyId id) const {
  return BodyAt(id).position;
}

float PhysicsWorld::Radius(PhysicsBodyId id) const {
  return BodyAt(id).radius;
}

Vec3F PhysicsWorld::WishVelocity(PhysicsBodyId id) const {
  return BodyAt(id).wish_velocity;
}

Vec3F PhysicsWorld::Velocity(PhysicsBodyId id) const {
  return BodyAt(id).last_result.velocity;
}

Vec3F PhysicsWorld::LastPositionCorrection(PhysicsBodyId id) const {
  return BodyAt(id).last_result.position_correction;
}

SphereBodySupport PhysicsWorld::Support(PhysicsBodyId id) const {
  if (id < 0 || static_cast<size_t>(id) >= bodies_.size() ||
      !bodies_[static_cast<size_t>(id)].alive) {
    return SphereBodySupport();
  }
  return bodies_[static_cast<size_t>(id)].last_result.support;
}

const std::vector<ContactPoint> &PhysicsWorld::Contacts(
    PhysicsBodyId id) const {
  if (id < 0 || static_cast<size_t>(id) >= bodies_.size() ||
      !bodies_[static_cast<size_t>(id)].alive) {
    return kInvalidContacts;
  }
  return bodies_[static_cast<size_t>(id)].manifold.Contacts();
}

ContactSolveOutcome PhysicsWorld::Outcome(PhysicsBodyId id) const {
  return BodyAt(id).last_result.outcome;
}

bool PhysicsWorld::SubstepBudgetExhausted(PhysicsBodyId id) const {
  return BodyAt(id).last_result.substep_budget_exhausted;
}

SweepSphereResult PhysicsWorld::SweepSphereQuery(const Vec3F &from,
    const Vec3F &to, float radius) const {
  return SweepSphere(soup_, from, to, radius, config_.skin);
}

bool PhysicsWorld::HeightBelowQuery(float x, float z, float from_y,
    float max_drop, float radius, float *out_height) const {
  return QuerySphereHeightBelow(soup_, x, z, from_y, max_drop, radius,
      config_.floor_normal_y, out_height);
}

SphereDropResult PhysicsWorld::DropSphereQuery(const Vec3F &start,
    float max_drop, float radius) const {
  return DropSphereBody(soup_, start, max_drop, radius, config_.skin);
}

bool PhysicsWorld::UnstickQuery(Vec3F *center, float radius) const {
  return UnstickSphereBody(soup_, center, radius);
}

}  // namespace arctic
