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

#define _USE_MATH_DEFINES
#include <cmath>
#include "quaternion.h"

namespace arctic {


QuaternionF::QuaternionF() {
  x = 0.f;
  y = 0.f;
  z = 0.f;
  w = 1.f;
}

QuaternionF::QuaternionF(float x_, float y_, float z_, float w_) {
	x = x_;
	y = y_;
	z = z_;
  w = w_;
}

QuaternionF::QuaternionF(const QuaternionF& b) {
	w = b.w; x = b.x; y = b.y; z = b.z;
}

QuaternionF::QuaternionF(const Vec3F& unit_axis, float angle) {
	float halfAngle = 0.5f*angle;
	float sinHalfAngle = ::sin(halfAngle);
	w = ::cos(halfAngle);
	x = sinHalfAngle*unit_axis.x;
	y = sinHalfAngle*unit_axis.y;
	z = sinHalfAngle*unit_axis.z;
}

QuaternionF::~QuaternionF() {
}

QuaternionF& QuaternionF::operator= (const QuaternionF& b) {
	w = b.w;
	x = b.x;
	y = b.y;
	z = b.z;
	return *this;
}

QuaternionF QuaternionF::operator+ (const QuaternionF& b) const {
	return QuaternionF(x + b.x, y + b.y, z + b.z, w + b.w);
}

QuaternionF QuaternionF::operator- (const QuaternionF& b) const {
	return QuaternionF(x - b.x, y - b.y, z - b.z, w - b.w);
}

QuaternionF QuaternionF::operator* (const float b) const {
	return QuaternionF(x*b, y*b, z*b, w*b);
}

QuaternionF QuaternionF::operator* (const QuaternionF& b) const {
	return QuaternionF(
		w*b.x + x*b.w + y*b.z - z*b.y,
		w*b.y + y*b.w + z*b.x - x*b.z,
		w*b.z + z*b.w + x*b.y - y*b.x,
    w*b.w - x*b.x - y*b.y - z*b.z);
}
void QuaternionF::ToAxisAngle(Vec3F& out_axis, float& out_angle) const {
	float lengthSquared = x*x + y*y + z*z;
	if (lengthSquared > 0.0f) {
		float clamped_w = w < -1.0f ? -1.0f : (w > 1.0f ? 1.0f : w);
		out_angle = 2.0f*acos(clamped_w);
		float inv = 1.0f / sqrtf(lengthSquared);
		out_axis.x = x*inv;
		out_axis.y = y*inv;
		out_axis.z = z*inv;
	} else {
		out_angle = 0.0f;
		out_axis.x = 1.0f;
		out_axis.y = 0.0f;
		out_axis.z = 0.0f;
	}
}

Mat33F QuaternionF::ToMat33F() const {
	return Mat33F(
		w*w + x*x - y*y - z*z, 2.0f*x*y - 2.0f*w*z, 2.0f*x*z + 2.0f*w*y,
		2.0f*x*y + 2.0f*w*z, w*w - x*x + y*y - z*z, 2.0f*y*z - 2.0f*w*x,
		2.0f*x*z - 2.0f*w*y, 2.0f*y*z + 2.0f*w*x, w*w - x*x - y*y + z*z);
}

void QuaternionF::ToPartialMatrix33F(Mat33F& out) const {
	out.m[0] = w*w + x*x - y*y - z*z; out.m[1] = 2.0f*x*y - 2.0f*w*z; out.m[2] = 2.0f*x*z + 2.0f*w*y;
	out.m[3] = 2.0f*x*y + 2.0f*w*z; out.m[4] = w*w - x*x + y*y - z*z; out.m[5] = 2.0f*y*z - 2.0f*w*x;
  out.m[6] = 2.0f*x*z - 2.0f*w*y; out.m[7] = 2.0f*y*z + 2.0f*w*x; out.m[8] = w*w - x*x - y*y + z*z;
}

void QuaternionF::Normalize() {
	float modulus = ::sqrt(w*w + x*x + y*y + z*z);
	float inv = 1.0f / modulus;
	x *= inv;
	y *= inv;
	z *= inv;
  w *= inv;
}

float QuaternionF::Norm() const {
	return x*x + y*y + z*z + w*w;
}

float QuaternionF::Modulus() const {
  return ::sqrt(x*x + y*y + z*z + w*w);
}

Vec3F QuaternionF::Rotate(const Vec3F& a) const {
	return ToMat33F()*a;
}

void QuaternionF::Clear() {
  x = 0.f;
  y = 0.f;
  z = 0.f;
  w = 1.f;
}


QuaternionF operator* (float a, const QuaternionF& b) {
	return QuaternionF(a*b.x, a*b.y, a*b.z, a*b.w);
}

QuaternionF Normalize(const QuaternionF& a) {
	float modulus = ::sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
	float inv = 1.0f / modulus;
	return QuaternionF(a.x*inv, a.y*inv, a.z*inv, a.w*inv);
}

QuaternionF Inverse(const QuaternionF& a) {
	float norm = (a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
	if (norm < 1e-30f) {
		return QuaternionF(0.f, 0.f, 0.f, 1.f);
	}
	float inv = 1.0f / norm;
	return QuaternionF(-a.x * inv, -a.y * inv, -a.z * inv, a.w * inv);
}

QuaternionF Conjugate(const QuaternionF& a) {
	return QuaternionF(-a.x, -a.y, -a.z, a.w);
}

QuaternionF slerp(QuaternionF const &a, QuaternionF const &b, float t) {
	QuaternionF na = Normalize(a);
	QuaternionF nb = Normalize(b);

	float dotProduct = na.x*nb.x + na.y*nb.y + na.z*nb.z + na.w*nb.w;

	// If dot product is negative, negate one quaternion to take the shorter arc.
	if (dotProduct < 0.0f) {
		nb = nb * -1.0f;
		dotProduct = -dotProduct;
	}

	const float THRESHOLD = 0.9995f;
	if (dotProduct > THRESHOLD) {
		return Normalize(na + t*(nb - na));
	}

	dotProduct = (dotProduct > 1.0f ? 1.0f : dotProduct);
	float alpha = acos(dotProduct);
	float beta = alpha*t;

	QuaternionF c = Normalize(nb - na*dotProduct);
	return na*::cos(beta) + c*::sin(beta);
}


}  // namespace arctic
