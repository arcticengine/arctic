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

#include "engine/physics_solver.h"

#include <algorithm>
#include <cmath>

namespace arctic {

namespace {

const Si32 kMaxBlockConstraints = 3;

// Gaussian elimination with partial pivoting for a k x k system, k <= 3.
// Returns false when the system is singular (parallel or otherwise
// linearly dependent normals in the active set), in which case the caller
// treats that subset as not independently solvable.
bool SolveSmallSystem(float m[3][3], const float *rhs, Si32 k, float *out_x) {
  float a[3][4];
  for (Si32 r = 0; r < k; ++r) {
    for (Si32 c = 0; c < k; ++c) {
      a[r][c] = m[r][c];
    }
    a[r][k] = rhs[r];
  }
  for (Si32 col = 0; col < k; ++col) {
    Si32 piv = col;
    float best = std::fabs(a[col][col]);
    for (Si32 r = col + 1; r < k; ++r) {
      if (std::fabs(a[r][col]) > best) {
        best = std::fabs(a[r][col]);
        piv = r;
      }
    }
    if (best < 1.0e-6f) {
      return false;
    }
    if (piv != col) {
      for (Si32 c = 0; c <= k; ++c) {
        std::swap(a[col][c], a[piv][c]);
      }
    }
    float d = a[col][col];
    for (Si32 c = col; c <= k; ++c) {
      a[col][c] /= d;
    }
    for (Si32 r = 0; r < k; ++r) {
      if (r == col) {
        continue;
      }
      float f = a[r][col];
      if (f == 0.0f) {
        continue;
      }
      for (Si32 c = col; c <= k; ++c) {
        a[r][c] -= f * a[col][c];
      }
    }
  }
  for (Si32 r = 0; r < k; ++r) {
    out_x[r] = a[r][k];
  }
  return true;
}

ContactSolveResult SolveExact(const Vec3F &wish,
    const SpeculativeConstraint *cs, Si32 count, float eps) {
  ContactSolveResult best;
  bool has_best = false;
  float best_dist2 = 0.0f;
  Si32 idx[kMaxBlockConstraints];

  Si32 subset_count = 1 << count;
  for (Si32 mask = 0; mask < subset_count; ++mask) {
    Si32 k = 0;
    for (Si32 i = 0; i < count; ++i) {
      if (mask & (1 << i)) {
        idx[k++] = i;
      }
    }
    Vec3F v = wish;
    float lambda[kMaxBlockConstraints] = {0.0f, 0.0f, 0.0f};
    if (k > 0) {
      float m[3][3];
      float rhs[3];
      for (Si32 r = 0; r < k; ++r) {
        const Vec3F &nr = cs[idx[r]].normal;
        for (Si32 c = 0; c < k; ++c) {
          m[r][c] = Dot(nr, cs[idx[c]].normal);
        }
        rhs[r] = Dot(nr, wish) - cs[idx[r]].bound;
      }
      if (!SolveSmallSystem(m, rhs, k, lambda)) {
        continue;
      }
      bool nonneg = true;
      for (Si32 r = 0; r < k; ++r) {
        if (lambda[r] < -eps) {
          nonneg = false;
          break;
        }
      }
      if (!nonneg) {
        continue;
      }
      for (Si32 r = 0; r < k; ++r) {
        v -= cs[idx[r]].normal * lambda[r];
      }
    }
    bool feasible = true;
    for (Si32 i = 0; i < count; ++i) {
      if (mask & (1 << i)) {
        continue;
      }
      if (Dot(cs[i].normal, v) > cs[i].bound + eps) {
        feasible = false;
        break;
      }
    }
    if (!feasible) {
      continue;
    }
    Vec3F d = v - wish;
    float dist2 = LengthSquared(d);
    if (!has_best || dist2 < best_dist2) {
      has_best = true;
      best_dist2 = dist2;
      best.velocity = v;
      best.active_mask = static_cast<Ui32>(mask);
      switch (k) {
        case 0:
          best.outcome = ContactSolveOutcome::kFree;
          break;
        case 1:
          best.outcome = ContactSolveOutcome::kFace;
          break;
        case 2:
          best.outcome = ContactSolveOutcome::kEdge;
          break;
        default:
          best.outcome = ContactSolveOutcome::kCorner;
          break;
      }
    }
  }
  if (!has_best) {
    // No subset of active constraints yields a point that also satisfies
    // every other constraint: the half-spaces close in on themselves with
    // no shared tangent direction to slide along. A random pick among the
    // constraints would move the body into whichever one was checked
    // last; a full stop is the only answer that violates none of them by
    // more than the state already had.
    best.velocity = Vec3F(0.0f, 0.0f, 0.0f);
    Ui32 mask = 0;
    for (Si32 i = 0; i < count; ++i) {
      mask |= (1u << i);
    }
    best.active_mask = mask;
    best.outcome = ContactSolveOutcome::kDegenerate;
  }
  return best;
}

ContactSolveResult SolvePgs(const Vec3F &wish,
    const SpeculativeConstraint *cs, Si32 count, Si32 iterations) {
  ContactSolveResult result;
  result.outcome = ContactSolveOutcome::kPgs;
  Vec3F v = wish;
  Ui32 mask = 0;
  for (Si32 iter = 0; iter < iterations; ++iter) {
    bool changed = false;
    for (Si32 i = 0; i < count; ++i) {
      float excess = Dot(cs[i].normal, v) - cs[i].bound;
      if (excess > 0.0f) {
        v -= cs[i].normal * excess;
        if (i < 32) {
          mask |= (1u << i);
        }
        changed = true;
      }
    }
    if (!changed) {
      break;
    }
  }
  result.velocity = v;
  result.active_mask = mask;
  return result;
}

}  // namespace

ContactSolveResult SolveContactVelocity(const Vec3F &wish_velocity,
    const SpeculativeConstraint *constraints, Si32 count) {
  if (count <= 0 || !constraints) {
    ContactSolveResult free_result;
    free_result.velocity = wish_velocity;
    return free_result;
  }
  float eps = 1.0e-5f * std::max(1.0f, Length(wish_velocity));
  if (count <= kMaxBlockConstraints) {
    return SolveExact(wish_velocity, constraints, count, eps);
  }
  return SolvePgs(wish_velocity, constraints, count, 8);
}

}  // namespace arctic
