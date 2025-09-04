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

#include "transform3f.h"

namespace arctic {

Transform3F::Transform3F() {
	Clear();
}

Transform3F::Transform3F(const Vec3F& displacement_, const QuaternionF& rotation_) {
	displacement = displacement_;
	rotation = rotation_;
}

Transform3F Transform3F::Transform(const Transform3F& a) const {
	return Transform3F(rotation.Rotate(a.displacement) + displacement, rotation * a.rotation);
}

Vec3F Transform3F::Transform(const Vec3F& a) const {
	return rotation.Rotate(a) + displacement;
}

QuaternionF Transform3F::Transform(const QuaternionF& a) const {
	return rotation * a;
}

void Transform3F::Clear() {
  displacement.x = 0.f;
  displacement.y = 0.f;
  displacement.z = 0.f;
  rotation.Clear();
  scale = 1.f;
}

/*Mat44F Transform3F::ToMatrix44F() const {
	Mat33F result;
	rotation.ToPartialMatrix33F(result);
	displacement.ToPartialMatrix33F(result);
	return result;
}*/

Transform3F Inverse(const Transform3F& a) {
	return Transform3F(a.displacement*-1.f, Inverse(a.rotation));
}

}  // namespace arctic
