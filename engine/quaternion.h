// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#pragma once

#include <engine/vec2f.h>
#include <engine/mat33f.h>

namespace arctic {

class QuaternionF {
 public:
	float w;
	float x;
	float y;
	float z;

	QuaternionF();
	QuaternionF(float w, float x_, float y_, float z_);
	QuaternionF(const QuaternionF& b);
	QuaternionF(const Vec3F& unit_axis, float angle);

	~QuaternionF();

	QuaternionF& operator= (const QuaternionF& b);
	QuaternionF operator+ (const QuaternionF& b) const;
	QuaternionF operator- (const QuaternionF& b) const;
	QuaternionF operator* (const float b) const;

	QuaternionF operator* (const QuaternionF& b) const;

	void Normalize();

	void ToAxisAngle(Vec3F& out_axis, float& out_angle) const;
	Mat33F ToMat33F() const;
	void ToPartialMatrix33F(Mat33F& out) const;
	
	Vec3F Rotate(const Vec3F& a) const;

	float Norm() const;
	float Modulus() const;

  void Clear();
};

QuaternionF operator* (float a, const QuaternionF& b);
QuaternionF Normalize(const QuaternionF& a);
QuaternionF Inverse(const QuaternionF& a);
QuaternionF Conjugate(const QuaternionF& a);
QuaternionF slerp(QuaternionF const &a, QuaternionF const &b, float t);


}  // namespace arctic
