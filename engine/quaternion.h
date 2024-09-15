// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2022 Huldra
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

#pragma once

#include <engine/vec2f.h>
#include <engine/mat33f.h>

namespace arctic {

/// @brief Represents a quaternion with floating-point components. Used for 3D rotations.
class QuaternionF {
 public:
	float x; ///< The x component of the quaternion.
	float y; ///< The y component of the quaternion.
	float z; ///< The z component of the quaternion.
  float w; ///< The w component of the quaternion.

	/// @brief Default constructor. Initializes a quaternion with all components set to 0.
	QuaternionF();

	/// @brief Constructor that initializes the quaternion with given components.
	/// @param x_ The x component.
	/// @param y_ The y component.
	/// @param z_ The z component.
	/// @param w_ The w component.
	QuaternionF(float x_, float y_, float z_, float w_);

	/// @brief Copy constructor.
	/// @param b The quaternion to copy.
	QuaternionF(const QuaternionF& b);

	/// @brief Constructor that creates a quaternion from an axis and an angle.
	/// @param unit_axis The axis of rotation (should be normalized).
	/// @param angle The angle of rotation in radians.
	QuaternionF(const Vec3F& unit_axis, float angle);

	/// @brief Destructor.
	~QuaternionF();

	/// @brief Assignment operator.
	/// @param b The quaternion to assign from.
	/// @return A reference to this quaternion after assignment.
	QuaternionF& operator= (const QuaternionF& b);

	/// @brief Addition operator.
	/// @param b The quaternion to add.
	/// @return The result of the addition.
	QuaternionF operator+ (const QuaternionF& b) const;

	/// @brief Subtraction operator.
	/// @param b The quaternion to subtract.
	/// @return The result of the subtraction.
	QuaternionF operator- (const QuaternionF& b) const;

	/// @brief Scalar multiplication operator.
	/// @param b The scalar to multiply by.
	/// @return The result of the multiplication.
	QuaternionF operator* (const float b) const;

	/// @brief Quaternion multiplication operator.
	/// @param b The quaternion to multiply with.
	/// @return The result of the multiplication.
	QuaternionF operator* (const QuaternionF& b) const;

	/// @brief Normalizes this quaternion in-place.
	void Normalize();

	/// @brief Converts the quaternion to axis-angle representation.
	/// @param out_axis The output axis of rotation.
	/// @param out_angle The output angle of rotation in radians.
	void ToAxisAngle(Vec3F& out_axis, float& out_angle) const;

	/// @brief Converts the quaternion to a 3x3 rotation matrix.
	/// @return The resulting 3x3 rotation matrix.
	Mat33F ToMat33F() const;

	/// @brief Converts the quaternion to a partial 3x3 rotation matrix.
	/// @param out The output 3x3 matrix.
	void ToPartialMatrix33F(Mat33F& out) const;
	
	/// @brief Rotates a vector by this quaternion.
	/// @param a The vector to rotate.
	/// @return The rotated vector.
	Vec3F Rotate(const Vec3F& a) const;

	/// @brief Calculates the norm (length squared) of the quaternion.
	/// @return The norm of the quaternion.
	float Norm() const;

	/// @brief Calculates the modulus (length) of the quaternion.
	/// @return The modulus of the quaternion.
	float Modulus() const;

  /// @brief Sets all components of the quaternion to zero.
  void Clear();
};

/// @brief Scalar multiplication operator (scalar on the left). 
/// @param a The scalar to multiply by.
/// @param b The quaternion to multiply.
/// @return The result of the multiplication.
QuaternionF operator* (float a, const QuaternionF& b);

/// @brief Normalizes a quaternion. The rotation represented by the quaternion is unchanged.
/// @param a The quaternion to normalize.
/// @return The normalized quaternion.
QuaternionF Normalize(const QuaternionF& a);

/// @brief Computes the inverse of a quaternion.
/// @param a The quaternion to invert.
/// @return The inverted quaternion.
QuaternionF Inverse(const QuaternionF& a);

/// @brief Computes the conjugate of a quaternion.
/// @param a The quaternion to conjugate.
/// @return The conjugated quaternion.
QuaternionF Conjugate(const QuaternionF& a);

/// @brief Performs spherical linear interpolation between two quaternions.
/// @param a The starting quaternion.
/// @param b The ending quaternion.
/// @param t The interpolation parameter (0 <= t <= 1).
/// @return The interpolated quaternion.
QuaternionF slerp(QuaternionF const &a, QuaternionF const &b, float t);


}  // namespace arctic
