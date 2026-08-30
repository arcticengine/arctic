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

#include "engine/physics_debug.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace arctic {

namespace {

std::string Sprint(const char *format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  return std::string(buf);
}

bool TriangleTouchesBox(const CollisionTriangle &tri, const Vec3F &at,
    float radius) {
  float lo_x = std::min(tri.a.x, std::min(tri.b.x, tri.c.x));
  float hi_x = std::max(tri.a.x, std::max(tri.b.x, tri.c.x));
  float lo_y = std::min(tri.a.y, std::min(tri.b.y, tri.c.y));
  float hi_y = std::max(tri.a.y, std::max(tri.b.y, tri.c.y));
  float lo_z = std::min(tri.a.z, std::min(tri.b.z, tri.c.z));
  float hi_z = std::max(tri.a.z, std::max(tri.b.z, tri.c.z));
  if (hi_x < at.x - radius || lo_x > at.x + radius) {
    return false;
  }
  if (hi_y < at.y - radius || lo_y > at.y + radius) {
    return false;
  }
  if (hi_z < at.z - radius || lo_z > at.z + radius) {
    return false;
  }
  return true;
}

}  // namespace

std::string FormatVec3FLiteral(const Vec3F &v) {
  return Sprint("Vec3F(%.9gf, %.9gf, %.9gf)", v.x, v.y, v.z);
}

std::vector<std::string> FormatCollideSoupNear(const CollideSoup &soup,
    const Vec3F &at, float radius) {
  std::vector<std::string> lines;
  lines.push_back(Sprint("SOUP near %s radius=%.9g of %d triangles",
      FormatVec3FLiteral(at).c_str(), radius, soup.TriangleCount()));
  Si32 found = 0;
  for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
    const CollisionTriangle &tri = soup.Triangle(i);
    if (!TriangleTouchesBox(tri, at, radius)) {
      continue;
    }
    ++found;
    lines.push_back(Sprint("SOUP tri=%d tag=%u grip=%.9g "
        "n=(%.9g,%.9g,%.9g) MakeTri(%s, %s, %s)",
        i, soup.Material(i).tag, soup.Material(i).grip,
        tri.n.x, tri.n.y, tri.n.z,
        FormatVec3FLiteral(tri.a).c_str(),
        FormatVec3FLiteral(tri.b).c_str(),
        FormatVec3FLiteral(tri.c).c_str()));
  }
  lines.push_back(Sprint("SOUP found=%d", found));
  return lines;
}

std::vector<std::string> FormatContacts(const CollideSoup &soup,
    const std::vector<ContactPoint> &contacts, const std::string &prefix) {
  std::vector<std::string> lines;
  lines.push_back(prefix + Sprint("contacts=%d",
      static_cast<Si32>(contacts.size())));
  for (const ContactPoint &c : contacts) {
    lines.push_back(prefix + Sprint("tri=%d kind=%d sub=%d separation=%.9g "
        "normal=(%.9g,%.9g,%.9g) constraint_normal=(%.9g,%.9g,%.9g)",
        c.feature.tri_index, static_cast<int>(c.feature.kind), c.feature.sub,
        c.separation, c.normal.x, c.normal.y, c.normal.z,
        c.constraint_normal.x, c.constraint_normal.y, c.constraint_normal.z));
    if (c.feature.tri_index < 0
        || c.feature.tri_index >= soup.TriangleCount()) {
      continue;
    }
    const CollisionTriangle &tri = soup.Triangle(c.feature.tri_index);
    lines.push_back(prefix + Sprint("  MakeTri(%s, %s, %s)",
        FormatVec3FLiteral(tri.a).c_str(),
        FormatVec3FLiteral(tri.b).c_str(),
        FormatVec3FLiteral(tri.c).c_str()));
  }
  return lines;
}

std::vector<std::string> FormatSphereBodyFrame(const CollideSoup &soup,
    const Vec3F &center, float radius, const Vec3F &wish_velocity,
    const SphereStepResult &result,
    const std::vector<ContactPoint> &contacts) {
  std::vector<std::string> lines;
  lines.push_back(Sprint("BODY center=%s radius=%.9g",
      FormatVec3FLiteral(center).c_str(), radius));
  lines.push_back(Sprint("BODY wish=%s solved=%s correction=%s",
      FormatVec3FLiteral(wish_velocity).c_str(),
      FormatVec3FLiteral(result.velocity).c_str(),
      FormatVec3FLiteral(result.position_correction).c_str()));
  lines.push_back(Sprint("BODY outcome=%d contact_count=%d "
      "substep_budget_exhausted=%d", static_cast<int>(result.outcome),
      result.contact_count, result.substep_budget_exhausted ? 1 : 0));
  const SphereBodySupport &sup = result.support;
  lines.push_back(Sprint("BODY has_floor=%d has_wall=%d has_ceiling=%d "
      "floor_n=(%.9g,%.9g,%.9g) floor_grip=%.9g floor_tag=%u "
      "has_floor_below=%d floor_distance=%.9g",
      sup.has_floor ? 1 : 0, sup.has_wall ? 1 : 0, sup.has_ceiling ? 1 : 0,
      sup.floor_normal.x, sup.floor_normal.y, sup.floor_normal.z,
      sup.floor_material.grip, sup.floor_material.tag,
      sup.has_floor_below ? 1 : 0, sup.floor_distance));
  std::vector<std::string> contact_lines = FormatContacts(soup, contacts,
      "BODY ");
  lines.insert(lines.end(), contact_lines.begin(), contact_lines.end());
  return lines;
}

std::vector<std::string> FormatSphereBodyFrame(const PhysicsWorld &world,
    PhysicsBodyId id) {
  SphereStepResult result;
  result.velocity = world.Velocity(id);
  result.position_correction = world.LastPositionCorrection(id);
  result.outcome = world.Outcome(id);
  result.support = world.Support(id);
  result.substep_budget_exhausted = world.SubstepBudgetExhausted(id);
  result.contact_count = static_cast<Si32>(world.Contacts(id).size());
  return FormatSphereBodyFrame(world.Soup(), world.Position(id),
      world.Radius(id), world.WishVelocity(id), result, world.Contacts(id));
}

Si32 CollideSoupFixture::Add(const Vec3F &a, const Vec3F &b, const Vec3F &c,
    const PhysicsMaterial &material) {
  CollisionTriangle tri;
  if (!tri.Set(a, b, c)) {
    problems_.push_back(Sprint("face %d is degenerate and will be dropped "
        "from the mesh: MakeTri(%s, %s, %s)",
        static_cast<Si32>(tris_.size()), FormatVec3FLiteral(a).c_str(),
        FormatVec3FLiteral(b).c_str(), FormatVec3FLiteral(c).c_str()));
    return -1;
  }
  tris_.push_back(tri);
  materials_.push_back(material);
  return static_cast<Si32>(tris_.size()) - 1;
}

Si32 CollideSoupFixture::AddFacing(const Vec3F &a, const Vec3F &b,
    const Vec3F &c, const Vec3F &expected_normal,
    const PhysicsMaterial &material) {
  Si32 index = Add(a, b, c, material);
  if (index < 0) {
    return index;
  }
  Vec3F want = NormalizeSafe(expected_normal);
  Vec3F got = NormalizeSafe(tris_[static_cast<size_t>(index)].n);
  if (Dot(want, got) < 0.99f) {
    problems_.push_back(Sprint("face %d faces (%.9g,%.9g,%.9g) but was "
        "meant to face (%.9g,%.9g,%.9g); check the order of its vertices",
        index, got.x, got.y, got.z, want.x, want.y, want.z));
  }
  return index;
}

std::string CollideSoupFixture::Problems() const {
  std::string out;
  for (size_t i = 0; i < problems_.size(); ++i) {
    out += problems_[i];
    out += "\n";
  }
  return out;
}

std::vector<std::string> CollideSoupFixture::NormalLines() const {
  std::vector<std::string> lines;
  for (size_t i = 0; i < tris_.size(); ++i) {
    Vec3F n = NormalizeSafe(tris_[i].n);
    lines.push_back(Sprint("FIXTURE face %d n=(%.9g,%.9g,%.9g) tag=%u",
        static_cast<Si32>(i), n.x, n.y, n.z, materials_[i].tag));
  }
  return lines;
}

void CollideSoupFixture::Build(CollideSoup *soup,
    Si32 target_cells_per_axis) {
  if (soup == nullptr) {
    return;
  }
  std::vector<Vec3F> pa, pb, pc;
  pa.reserve(tris_.size());
  pb.reserve(tris_.size());
  pc.reserve(tris_.size());
  for (size_t i = 0; i < tris_.size(); ++i) {
    pa.push_back(tris_[i].a);
    pb.push_back(tris_[i].b);
    pc.push_back(tris_[i].c);
  }
  soup->Build(pa, pb, pc, materials_, target_cells_per_axis);
  if (soup->TriangleCount() != static_cast<Si32>(tris_.size())) {
    problems_.push_back(Sprint("the mesh kept %d of %d faces",
        soup->TriangleCount(), static_cast<Si32>(tris_.size())));
  }
}

}  // namespace arctic
