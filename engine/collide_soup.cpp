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

#include "engine/collide_soup.h"

#include <algorithm>
#include <cmath>

namespace arctic {

void CollideSoup::Clear() {
  tris_.clear();
  materials_.clear();
  bins_.clear();
  stamp_.clear();
  gen_ = 1;
  bounds_ = Bound3F(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  cell_ = 1.0f;
  width_ = 0;
  height_ = 0;
}

void CollideSoup::Build(const std::vector<Vec3F> &points_a,
    const std::vector<Vec3F> &points_b, const std::vector<Vec3F> &points_c,
    const std::vector<PhysicsMaterial> &materials,
    Si32 target_cells_per_axis) {
  Clear();
  size_t n = points_a.size();
  if (n != points_b.size() || n != points_c.size() ||
      n != materials.size()) {
    return;
  }
  tris_.reserve(n);
  materials_.reserve(n);
  bool has_bounds = false;
  for (size_t i = 0; i < n; ++i) {
    CollisionTriangle tri;
    if (!tri.Set(points_a[i], points_b[i], points_c[i])) {
      continue;
    }
    tris_.push_back(tri);
    materials_.push_back(materials[i]);
    Bound3F tb(tri.a, tri.a);
    tb = Include(tb, tri.b);
    tb = Include(tb, tri.c);
    bounds_ = has_bounds ? Include(bounds_, tb) : tb;
    has_bounds = true;
  }
  if (!has_bounds) {
    return;
  }
  const float kPad = 0.01f;
  bounds_ = Expand(bounds_, kPad);
  float span_x = bounds_.max_x - bounds_.min_x;
  float span_z = bounds_.max_z - bounds_.min_z;
  float span = std::max(span_x, span_z);
  if (span < 1.0e-3f) {
    span = 1.0e-3f;
  }
  if (target_cells_per_axis < 1) {
    target_cells_per_axis = 1;
  }
  cell_ = span / static_cast<float>(target_cells_per_axis);
  width_ = std::max(static_cast<Si32>(std::ceil(span_x / cell_)), 1);
  height_ = std::max(static_cast<Si32>(std::ceil(span_z / cell_)), 1);
  const Si32 kMaxCellsPerAxis = 4096;
  width_ = std::min(width_, kMaxCellsPerAxis);
  height_ = std::min(height_, kMaxCellsPerAxis);

  bins_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_),
      CollideGridBin());
  stamp_.assign(tris_.size(), 0);
  gen_ = 1;
  for (size_t i = 0; i < tris_.size(); ++i) {
    const CollisionTriangle &t = tris_[i];
    float minx = std::min(t.a.x, std::min(t.b.x, t.c.x));
    float maxx = std::max(t.a.x, std::max(t.b.x, t.c.x));
    float minz = std::min(t.a.z, std::min(t.b.z, t.c.z));
    float maxz = std::max(t.a.z, std::max(t.b.z, t.c.z));
    float miny = std::min(t.a.y, std::min(t.b.y, t.c.y));
    float maxy = std::max(t.a.y, std::max(t.b.y, t.c.y));
    Si32 x0 = CellX(minx);
    Si32 x1 = CellX(maxx);
    Si32 z0 = CellZ(minz);
    Si32 z1 = CellZ(maxz);
    // Both ends of each axis must be clamped into [0, width_-1] /
    // [0, height_-1], not just the end that is expected to need it: a
    // triangle whose AABB reaches past the grid on the min side ends up
    // with x0/z0 clamped up from a negative value while x1/z1 stay
    // in-range, which is harmless, but one that reaches past the grid on
    // the max side (e.g. its own bounds pushed the grid's cell_ down via a
    // huge target_cells_per_axis, or plain float slop at the mesh's own
    // edge) gets x1/z1 clamped down while x0/z0 is left unclamped and
    // greater than the now-clamped x1/z1. The insertion loop below only
    // runs while lo <= hi, so that triangle silently lands in zero cells
    // instead of the boundary row/column it should share with everything
    // else out there.
    x0 = std::max(x0, 0);
    x0 = std::min(x0, width_ - 1);
    x1 = std::max(x1, 0);
    x1 = std::min(x1, width_ - 1);
    z0 = std::max(z0, 0);
    z0 = std::min(z0, height_ - 1);
    z1 = std::max(z1, 0);
    z1 = std::min(z1, height_ - 1);
    for (Si32 iz = z0; iz <= z1; ++iz) {
      for (Si32 ix = x0; ix <= x1; ++ix) {
        CollideGridBin &bin = bins_[static_cast<size_t>(iz * width_ + ix)];
        if (bin.tris.empty()) {
          bin.min_y = miny;
          bin.max_y = maxy;
        } else {
          bin.min_y = std::min(bin.min_y, miny);
          bin.max_y = std::max(bin.max_y, maxy);
        }
        bin.tris.push_back(static_cast<Si32>(i));
      }
    }
  }
}

}  // namespace arctic
