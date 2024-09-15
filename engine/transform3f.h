// The MIT License (MIT)
//
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

#ifndef ENGINE_TRANSFORM3F_H_
#define ENGINE_TRANSFORM3F_H_

#include "engine/arctic_types.h"
#include "engine/quaternion.h"

namespace arctic {

/// @brief A 3D transform.
class Transform3F {
 public:
  Vec3F displacement; ///< The displacement of the transform.	
  QuaternionF rotation; ///< The rotation of the transform.
  float scale = 1.f; ///< The scale of the transform.

  /// @brief Default constructor.
  Transform3F();

  /// @brief Constructor.
  /// @param displacement_ The displacement of the transform.
  /// @param rotation_ The rotation of the transform.
  Transform3F(const Vec3F& displacement_, const QuaternionF& rotation_);

  /// @brief Transforms a transform using the transform.
  /// @param a The transform to transform.
  /// @return The transformed transform.
  Transform3F Transform(const Transform3F& a) const;

  /// @brief Transforms a point using the transform.
  /// @param a The point to transform.
  /// @return The transformed point.
	Vec3F Transform(const Vec3F& a) const;

  /// @brief Transforms a quaternion using the transform.
  /// @param a The quaternion to transform.
  /// @return The transformed quaternion.
	QuaternionF Transform(const QuaternionF& a) const;
	//Mat33F ToMatrix33F() const;

  /// @brief Clears the transform.
  void Clear();
};

/// @brief Inverts a transform.
/// @param a The transform to invert.
/// @return The inverted transform.
Transform3F Inverse(const Transform3F& a);

}  // namespace arctic

#endif  // ENGINE_TRANSFORM3F_H_
