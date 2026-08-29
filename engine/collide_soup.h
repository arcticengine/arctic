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

#ifndef ENGINE_COLLIDE_SOUP_H_
#define ENGINE_COLLIDE_SOUP_H_

#include <vector>

#include "engine/arctic_types.h"
#include "engine/bound3f.h"
#include "engine/physics_types.h"
#include "engine/sphere_vs_triangle.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// One cell of the broad-phase grid: the triangles whose XZ footprint
/// overlaps the cell, plus the Y range those triangles span. The Y range
/// lets a 3D query reject a whole cell without touching its triangle list
/// when the query box does not overlap it vertically, which matters for
/// tall multi-story geometry where an XZ-only grid would otherwise hand the
/// narrow-phase triangles from a completely different floor.
struct CollideGridBin {
  std::vector<Si32> tris;
  float min_y = 0.0f;
  float max_y = 0.0f;
};

/// A static triangle soup with a per-triangle material and a uniform XZ grid
/// for broad-phase queries. This is the engine home of the spatial index
/// Hover Racer used to hand-roll as ForCollideInAabb over g_collide_bins;
/// the grid here sizes itself from the mesh's own bounds rather than
/// borrowing a game's height-field resolution.
class CollideSoup {
 public:
  CollideSoup() {}

  /// Replaces the whole soup and rebuilds the broad-phase grid. triangles
  /// and materials must be the same length; a triangle whose three points
  /// are degenerate (CollisionTriangle::Set returns false) is dropped.
  /// target_cells_per_axis controls the grid resolution along the longer of
  /// the mesh's XZ extents; the other axis gets as many cells of the same
  /// size as its extent needs.
  void Build(const std::vector<Vec3F> &points_a,
      const std::vector<Vec3F> &points_b, const std::vector<Vec3F> &points_c,
      const std::vector<PhysicsMaterial> &materials,
      Si32 target_cells_per_axis = 256);

  void Clear();

  Si32 TriangleCount() const {
    return static_cast<Si32>(tris_.size());
  }
  const CollisionTriangle &Triangle(Si32 index) const {
    return tris_[static_cast<size_t>(index)];
  }
  const PhysicsMaterial &Material(Si32 index) const {
    return materials_[static_cast<size_t>(index)];
  }
  const Bound3F &Bounds() const {
    return bounds_;
  }

  /// Calls fn(Si32 triangle_index) once for every triangle whose grid cells
  /// overlap box, each triangle visited at most once per call. Triangles
  /// are not filtered further here: whether a candidate really touches the
  /// query shape is up to the narrow-phase.
  template <typename Fn>
  void ForEachNear(const Bound3F &box, Fn fn) const {
    if (tris_.empty() || bins_.empty()) {
      return;
    }
    Si32 x0 = CellX(box.min_x);
    Si32 x1 = CellX(box.max_x);
    Si32 z0 = CellZ(box.min_z);
    Si32 z1 = CellZ(box.max_z);
    // Clamp both ends of each axis into range, matching Build(): a query
    // box that reaches past the grid on the max side must not leave x1/z1
    // clamped down while x0/z0 stays past it unclamped, or the loop below
    // (which only runs while lo <= hi) silently visits nothing instead of
    // the boundary cells it should still overlap.
    if (x0 < 0) {
      x0 = 0;
    }
    if (x0 >= width_) {
      x0 = width_ - 1;
    }
    if (x1 < 0) {
      x1 = 0;
    }
    if (x1 >= width_) {
      x1 = width_ - 1;
    }
    if (z0 < 0) {
      z0 = 0;
    }
    if (z0 >= height_) {
      z0 = height_ - 1;
    }
    if (z1 < 0) {
      z1 = 0;
    }
    if (z1 >= height_) {
      z1 = height_ - 1;
    }
    ++gen_;
    if (gen_ == 0) {
      std::fill(stamp_.begin(), stamp_.end(), 0);
      gen_ = 1;
    }
    for (Si32 iz = z0; iz <= z1; ++iz) {
      for (Si32 ix = x0; ix <= x1; ++ix) {
        const CollideGridBin &bin = bins_[static_cast<size_t>(
            iz * width_ + ix)];
        if (bin.tris.empty()) {
          continue;
        }
        if (bin.max_y < box.min_y || bin.min_y > box.max_y) {
          continue;
        }
        for (Si32 idx : bin.tris) {
          if (stamp_[static_cast<size_t>(idx)] == gen_) {
            continue;
          }
          stamp_[static_cast<size_t>(idx)] = gen_;
          fn(idx);
        }
      }
    }
  }

  /// Convenience wrapper for a swept sphere: the query box is the segment
  /// from p0 to p1 expanded by radius on every axis.
  template <typename Fn>
  void ForEachNearSegment(const Vec3F &p0, const Vec3F &p1, float radius,
      Fn fn) const {
    Bound3F box(
        std::min(p0.x, p1.x) - radius, std::max(p0.x, p1.x) + radius,
        std::min(p0.y, p1.y) - radius, std::max(p0.y, p1.y) + radius,
        std::min(p0.z, p1.z) - radius, std::max(p0.z, p1.z) + radius);
    ForEachNear(box, fn);
  }

 private:
  Si32 CellX(float x) const {
    return static_cast<Si32>(std::floor((x - bounds_.min_x) / cell_));
  }
  Si32 CellZ(float z) const {
    return static_cast<Si32>(std::floor((z - bounds_.min_z) / cell_));
  }

  std::vector<CollisionTriangle> tris_;
  std::vector<PhysicsMaterial> materials_;
  std::vector<CollideGridBin> bins_;
  mutable std::vector<Si32> stamp_;
  mutable Si32 gen_ = 1;
  Bound3F bounds_ = Bound3F(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float cell_ = 1.0f;
  Si32 width_ = 0;
  Si32 height_ = 0;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_COLLIDE_SOUP_H_
