// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 Huldra
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

#ifndef ENGINE_BOUND2F_H_
#define ENGINE_BOUND2F_H_

#define _USE_MATH_DEFINES
#include <cmath>
#include "engine/arctic_types.h"
#include "engine/vec2f.h"

namespace arctic {

/// @addtogroup global_math
/// @{

/// @brief A 2D bounding box.
struct Bound2F {
  union {
    struct {
      float min_x; ///< The minimum x-coordinate of the bounding box.
      float max_x; ///< The maximum x-coordinate of the bounding box.
      float min_y; ///< The minimum y-coordinate of the bounding box.
      float max_y; ///< The maximum y-coordinate of the bounding box.
    };
    float element[4]; ///< The elements of the bounding box.
  };

  /// @brief Default constructor.
  Bound2F() {}

  /// @brief Constructor from minimum and maximum coordinates.
  /// @param mix The minimum x-coordinate.
  /// @param max The maximum x-coordinate.
  /// @param miy The minimum y-coordinate.
  /// @param may The maximum y-coordinate.
  explicit Bound2F(float mix, float max, float miy, float may) {
    min_x = mix;
    max_x = max;
    min_y = miy;
    max_y = may;
  }

  /// @brief Constructor for a symmetric bounding box.
  /// @param val The value to use as the magnitude of the bounding box size, positive or negative.
  explicit Bound2F(float val) {
    min_x = -val;
    max_x = val;
    min_y = -val;
    max_y = val;
  }

  /// @brief Constructor from an array of floats.
  /// @param v The array of floats. The order is min_x, max_x, min_y, max_y.
  explicit Bound2F(float const *const v) {
    min_x = v[0];
    max_x = v[1];
    min_y = v[2];
    max_y = v[3];
  }

  /// @brief Constructor from two arrays of floats.
  /// @param vmin The array of floats for the minimum coordinates. The order is min_x, min_y.
  /// @param vmax The array of floats for the maximum coordinates. The order is max_x, max_y.
  explicit Bound2F(float const *const vmin, float const *const vmax) {
    min_x = vmin[0];
    max_x = vmax[0];
    min_y = vmin[1];
    max_y = vmax[1];
  }

  /// @brief Constructor from two vectors.
  /// @param mi The minimum vector.
  /// @param ma The maximum vector.
  explicit Bound2F(const Vec2F &mi, const Vec2F &ma) {
    min_x = mi.x;
    max_x = ma.x;
    min_y = mi.y;
    max_y = ma.y;
  }

  /// @brief Access the elements of the bounding box.
  /// @param i The index of the element to access. The order is min_x, max_x, min_y, max_y.
  /// @return The element at the specified index.
  float &operator[](Si32 i) {
    return element[i];
  }

  /// @brief Access the elements of the bounding box.
  /// @param i The index of the element to access. The order is min_x, max_x, min_y, max_y.
  /// @return The element at the specified index.
  const float &operator[](Si32 i) const {
    return element[i];
  }
};

/// @brief Check if a point is inside a bounding box.
/// @param b The bounding box.
/// @param p The point.
/// @return True if the point is inside the bounding box, false otherwise.
inline bool Contains(Bound2F const &b, Vec2F const &p) {
  if (p.x < b.min_x) {
    return false;
  }
  if (p.y < b.min_y) {
    return false;
  }
  if (p.x > b.max_x) {
    return false;
  }
  if (p.y > b.max_y) {
    return false;
  }
  return true;
}
/// @}

}  // namespace arctic

#endif  // ENGINE_BOUND2F_H_
