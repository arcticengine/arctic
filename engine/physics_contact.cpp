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

#include "engine/physics_contact.h"

#include <algorithm>

namespace arctic {

ContactFeature ClassifyTriangleFeature(const CollisionTriangle &tri,
    const Vec3F &p, Si32 tri_index) {
  ContactFeature f;
  f.tri_index = tri_index;
  Vec3F ap = p - tri.a;
  float d1 = Dot(tri.ab, ap);
  float d2 = Dot(tri.ac, ap);
  if (d1 <= 0.0f && d2 <= 0.0f) {
    f.kind = ContactFeatureKind::kVertex;
    f.sub = 0;  // a
    return f;
  }
  Vec3F bp = p - tri.b;
  float d3 = Dot(tri.ab, bp);
  float d4 = Dot(tri.ac, bp);
  if (d3 >= 0.0f && d4 <= d3) {
    f.kind = ContactFeatureKind::kVertex;
    f.sub = 1;  // b
    return f;
  }
  float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
    f.kind = ContactFeatureKind::kEdge;
    f.sub = 0;  // ab
    return f;
  }
  Vec3F cp = p - tri.c;
  float d5 = Dot(tri.ab, cp);
  float d6 = Dot(tri.ac, cp);
  if (d6 >= 0.0f && d5 <= d6) {
    f.kind = ContactFeatureKind::kVertex;
    f.sub = 2;  // c
    return f;
  }
  float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
    f.kind = ContactFeatureKind::kEdge;
    f.sub = 2;  // ca
    return f;
  }
  float va = d3 * d6 - d5 * d4;
  if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
    f.kind = ContactFeatureKind::kEdge;
    f.sub = 1;  // bc
    return f;
  }
  f.kind = ContactFeatureKind::kFace;
  f.sub = 0;
  return f;
}

void PhysicsManifold::Update(std::vector<ContactPoint> *fresh) {
  if (!fresh) {
    contacts_.clear();
    return;
  }
  std::sort(fresh->begin(), fresh->end(),
      [](const ContactPoint &a, const ContactPoint &b) {
        if (a.feature.tri_index != b.feature.tri_index) {
          return a.feature.tri_index < b.feature.tri_index;
        }
        if (a.feature.kind != b.feature.kind) {
          return a.feature.kind < b.feature.kind;
        }
        return a.feature.sub < b.feature.sub;
      });
  for (ContactPoint &c : *fresh) {
    c.age = 0;
    for (const ContactPoint &prev : contacts_) {
      if (prev.feature == c.feature) {
        c.warm_speed = prev.warm_speed;
        c.age = prev.age + 1;
        break;
      }
    }
  }
  contacts_ = *fresh;
}

void PhysicsManifold::Clear() {
  contacts_.clear();
}

}  // namespace arctic
