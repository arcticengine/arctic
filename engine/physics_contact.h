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

#ifndef ENGINE_PHYSICS_CONTACT_H_
#define ENGINE_PHYSICS_CONTACT_H_

#include <vector>

#include "engine/arctic_types.h"
#include "engine/physics_types.h"
#include "engine/sphere_vs_triangle.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// Which part of a triangle a contact's closest point landed on. Two
/// contacts computed on different frames refer to the same physical feature
/// exactly when tri_index, kind and sub all match, regardless of how the
/// sphere center moved between those frames.
enum class ContactFeatureKind : Ui8 {
  kFace = 0,
  kEdge = 1,
  kVertex = 2,
};

/// sub selects which edge (0 = ab, 1 = bc, 2 = ca) or which vertex
/// (0 = a, 1 = b, 2 = c) within the triangle; unused (0) for a face contact.
struct ContactFeature {
  Si32 tri_index = -1;
  ContactFeatureKind kind = ContactFeatureKind::kFace;
  Si32 sub = 0;

  bool operator==(const ContactFeature &other) const {
    return tri_index == other.tri_index && kind == other.kind &&
        sub == other.sub;
  }
  bool operator!=(const ContactFeature &other) const {
    return !(*this == other);
  }
};

/// One narrow-phase result plus the bookkeeping a PhysicsManifold carries
/// across steps. normal is the geometric surface normal (sphere center
/// minus closest point, or the triangle's face normal when the sphere sits
/// exactly on the feature); constraint_normal is what the solver actually
/// enforces and may differ from normal (a wall's constraint_normal is the
/// horizontal projection of normal, so a wall contact never eats the
/// vertical part of the wish velocity, matching how Hover Racer always
/// treated its hover lift as independent of what the craft's sides touch).
struct ContactPoint {
  ContactFeature feature;
  Vec3F point = Vec3F(0.0f, 0.0f, 0.0f);
  Vec3F normal = Vec3F(0.0f, 1.0f, 0.0f);
  Vec3F constraint_normal = Vec3F(0.0f, 1.0f, 0.0f);
  /// Signed distance from the sphere's skin to the surface along normal:
  /// positive means free space, negative means the sphere already overlaps
  /// the feature by that much.
  float separation = 0.0f;
  PhysicsMaterial material;
  /// Last step's resolved closing speed against this same feature, carried
  /// over by PhysicsManifold::Update as a warm-start seed for the solver.
  float warm_speed = 0.0f;
  /// Number of consecutive steps (including this one) this exact feature
  /// has been part of the body's contact set; 0 the first time it appears.
  Si32 age = 0;
};

/// Computes which Voronoi region of tri the closest point to p falls in.
/// This mirrors CollisionTriangle::ClosestPoint's own branches exactly, so
/// the feature id it returns is the identity of whatever ClosestPoint(p)
/// would have returned.
ContactFeature ClassifyTriangleFeature(const CollisionTriangle &tri,
    const Vec3F &p, Si32 tri_index);

/// Per-body contact history. Update() takes this step's freshly detected
/// contacts, matches each one against the previous step's set by feature
/// id, carries over warm_speed and increments age on a match, sorts the
/// result into canonical order (by tri_index, then kind, then sub, so the
/// solver's answer does not depend on broad-phase traversal order), and
/// keeps that as the new persisted set. A feature that is not part of this
/// step's fresh list is simply dropped: persistence here only stabilizes
/// identity and seeds the solver, it never keeps a contact that the current
/// position no longer supports.
class PhysicsManifold {
 public:
  void Update(std::vector<ContactPoint> *fresh);
  void Clear();
  const std::vector<ContactPoint> &Contacts() const {
    return contacts_;
  }
  /// Lets a solver stamp its result (e.g. the realized closing speed) back
  /// onto the persisted contacts right after Update(), so the next call to
  /// Update() carries that value forward as warm_speed for whichever
  /// contacts match by feature id again.
  std::vector<ContactPoint> &MutableContacts() {
    return contacts_;
  }

 private:
  std::vector<ContactPoint> contacts_;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_PHYSICS_CONTACT_H_
