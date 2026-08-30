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

#ifndef ENGINE_PHYSICS_DEBUG_H_
#define ENGINE_PHYSICS_DEBUG_H_

#include <string>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/collide_soup.h"
#include "engine/physics_contact.h"
#include "engine/physics_sphere_body.h"
#include "engine/physics_types.h"
#include "engine/physics_world.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// Diagnostics for the sphere physics. Every function here returns lines of
/// text rather than printing them, so a caller sends them wherever its own
/// logging goes and a test can assert on them.
///
/// Two things are worth having in the engine rather than in each game.
/// First, when a body misbehaves on a particular piece of a mesh, the way to
/// turn that into a repeatable test is to copy the geometry around it into a
/// fixture, and doing that by hand invites typos in nine-digit coordinates.
/// Second, deciding what to look at requires the contact set, the constraint
/// normals and the triangles behind them together in one place; assembling
/// that by hand is how Hover Racer ended up with a private copy of half of
/// this file.

/// "Vec3F(1.5f, 0f, -3f)"-style text, with enough digits to survive a round
/// trip through a float.
std::string FormatVec3FLiteral(const Vec3F &v);

/// One line per triangle whose bounding box overlaps the box of half-size
/// `radius` around `at`: the triangle's index, material tag, face normal and
/// a ready-to-paste MakeTri(...) call with its three points. Every triangle
/// of the soup is considered, not just the ones the broad-phase grid offers,
/// so a dump is still trustworthy when the grid itself is the suspect.
std::vector<std::string> FormatCollideSoupNear(const CollideSoup &soup,
    const Vec3F &at, float radius);

/// One line per contact: feature id, separation, geometric normal, the
/// normal the solver was given (they differ for walls, which is exactly the
/// kind of thing worth seeing), and the vertices of the triangle behind it.
/// `prefix` starts each line, so a caller can tell this frame's contacts
/// from the previous frame's in one log.
std::vector<std::string> FormatContacts(const CollideSoup &soup,
    const std::vector<ContactPoint> &contacts, const std::string &prefix);

/// A whole frame of one body: where it is, what it was asked to do, what the
/// solver allowed, what the cleanup pass corrected, what it is supported by,
/// and all of its contacts.
std::vector<std::string> FormatSphereBodyFrame(const CollideSoup &soup,
    const Vec3F &center, float radius, const Vec3F &wish_velocity,
    const SphereStepResult &result,
    const std::vector<ContactPoint> &contacts);

/// The same for a body registered with a world, taken from its last step.
std::vector<std::string> FormatSphereBodyFrame(const PhysicsWorld &world,
    PhysicsBodyId id);

/// Collects triangles for a hand-written fixture mesh and checks the two
/// things that otherwise break one silently. A degenerate triangle is
/// dropped by CollideSoup::Build without a word, so the fixture quietly has
/// fewer faces than it reads as having. A winding order that points a face
/// the wrong way is worse: the mesh looks right, and the failure comes out
/// as a sphere falling through a floor or ignoring a wall, several layers
/// away from the typo. Reproducing a bug from a game log means transcribing
/// nine-digit coordinates, which is exactly when both happen.
///
/// Usage: add faces, say which way each is meant to point, assert Ok() with
/// Problems() as the message, then Build a soup from it.
class CollideSoupFixture {
 public:
  /// Adds a face. Returns its index, or -1 when the three points are
  /// degenerate (which is also recorded in Problems()).
  Si32 Add(const Vec3F &a, const Vec3F &b, const Vec3F &c,
      const PhysicsMaterial &material = PhysicsMaterial());
  /// Adds a face that must point roughly along `expected_normal`; a face
  /// pointing the other way is recorded in Problems() as a winding order to
  /// check, with both normals printed.
  Si32 AddFacing(const Vec3F &a, const Vec3F &b, const Vec3F &c,
      const Vec3F &expected_normal,
      const PhysicsMaterial &material = PhysicsMaterial());

  bool Ok() const {
    return problems_.empty();
  }
  /// Everything wrong with the fixture, one problem per line, empty when
  /// there is nothing wrong. Meant to be handed straight to a test's
  /// failure message.
  std::string Problems() const;
  /// One line per face with its normal, for eyeballing a fixture.
  std::vector<std::string> NormalLines() const;

  Si32 Count() const {
    return static_cast<Si32>(tris_.size());
  }
  const CollisionTriangle &Triangle(Si32 index) const {
    return tris_[static_cast<size_t>(index)];
  }
  /// Builds the soup, and reports through Problems() if the soup ended up
  /// with fewer triangles than were added.
  void Build(CollideSoup *soup, Si32 target_cells_per_axis = 8);

 private:
  std::vector<CollisionTriangle> tris_;
  std::vector<PhysicsMaterial> materials_;
  std::vector<std::string> problems_;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_PHYSICS_DEBUG_H_
