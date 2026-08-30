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

#ifndef ENGINE_PHYSICS_TYPES_H_
#define ENGINE_PHYSICS_TYPES_H_

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_physics
/// @{

/// Handle to a body registered with a PhysicsWorld. Bodies are addressed by
/// this id rather than by pointer so the world is free to move them around
/// internally.
using PhysicsBodyId = Si32;
const PhysicsBodyId kInvalidPhysicsBodyId = -1;

/// Per-triangle (or per-body) surface properties the game reads back after
/// a step. The engine itself only ever forwards this value; it never
/// interprets grip or tag.
struct PhysicsMaterial {
  float grip = 1.0f;
  Ui32 tag = 0;
};

/// Tunable knobs for PhysicsWorld::Step. Defaults reproduce the constants
/// Hover Racer tuned by hand (kContactSkin, kRideStickyFrames's replacement,
/// the old steps-from-speed substepping).
struct PhysicsStepConfig {
  /// A substep may cover at most this much distance; a step whose wish
  /// velocity would move a body further is split into several substeps so a
  /// single sweep does not have to reason about the whole large motion.
  float max_substep_move = 1.15f;
  /// Hard cap on substeps per Step() call regardless of how far a body asked
  /// to move, so a runaway wish velocity cannot spend unbounded time.
  Si32 max_substeps = 8;
  /// Contacts are created for any feature within (radius + skin) of the
  /// sphere's surface. This is the speculative margin: a face this close is
  /// constrained continuously (n*v' <= separation/dt) rather than being
  /// included or excluded by a binary threshold, which is what used to make
  /// the contact set flicker between frames.
  float skin = 0.06f;
  /// Contacts already overlapping deeper than this are pushed out by a
  /// dedicated position-correction pass instead of the velocity solve, so
  /// resolving a deep penetration never injects energy into the velocity.
  float max_position_correction = 4.0f;
  /// Fraction of the remaining penetration corrected per step; matches the
  /// old UnstickSphere's iterative shallow-first push but expressed as a
  /// single continuous rate instead of a fixed 20-iteration loop.
  float position_correction_rate = 0.2f;
  /// PGS relaxation passes used only when more than 3 contacts are active at
  /// once and the exact block solver no longer applies.
  Si32 pgs_iterations = 8;
  /// A triangle whose face normal's Y component is at or above this value
  /// is a floor; at or below its negation it is a ceiling; in between it is
  /// a wall. Matches the threshold Hover Racer tuned by hand.
  float floor_normal_y = 0.45f;
  /// How far below a body to measure the surface it is riding over, on top
  /// of the contacts. Contacts only ever see what is within reach, so they
  /// cannot answer "how far above the ground am I" for a body in the air --
  /// and a game that hovers needs exactly that, far enough out to start
  /// braking before it arrives. Set this to the tallest drop the hover
  /// control must brake for and read SphereBodySupport::floor_distance
  /// after the step; leave it at zero and no measurement is taken, so a
  /// caller with no use for it pays nothing.
  float support_probe = 0.0f;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_PHYSICS_TYPES_H_
