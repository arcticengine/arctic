// The MIT License (MIT)
//
// Copyright (c) 2020 Huldra
// Copyright (c) 2021 The Lasting Curator
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
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

#ifndef ENGINE_TRANSFORM2F_H_
#define ENGINE_TRANSFORM2F_H_

#include "engine/arctic_types.h"
#include "engine/dual_complex.h"

namespace arctic {

/// @brief A 2D transform.
struct Transform2F {
  DualComplexF dc; ///< The dual complex number, which is a combination of translation and rotation.
  float scale = 1.f; ///< The scale of the transform.

  /// @brief Sets the position of the transform.
  /// @param position The position to set.
  void SetPosition(Vec2F position) {
    dc.dual_x = position.x * 0.5f;
    dc.dual_y = position.y * 0.5f;
  }

  /// @brief Sets the position of the transform.
  /// @param x The x position to set.
  /// @param y The y position to set.
  void SetPosition(float x, float y) {
    dc.dual_x = x * 0.5f;
    dc.dual_y = y * 0.5f;
  }

  /// @brief Transforms a point using the transform.
  /// @param point The point to transform.
  /// @return The transformed point.
  Vec2F Transform(Vec2F point) const {
    return scale * dc.Transform(point);
  }
};

}  // namespace arctic

#endif  // ENGINE_TRANSFORM2F_H_
