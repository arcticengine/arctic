// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 - 2021 Huldra
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

#ifndef ENGINE_SCALAR_MATH_H_
#define ENGINE_SCALAR_MATH_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_math
/// @{

inline Si32 Clamp(const Si32 v, const Si32 mi, const Si32 ma) {
  return (v < mi) ? mi : ((v > ma) ? ma : v);
}

inline float Clamp(const float v, const float mi, const float ma) {
    return (v < mi) ? mi : ((v > ma) ? ma : v);
}

inline float Clamp(const double v, const double mi, const double ma) {
    return (v < mi) ? mi : ((v > ma) ? ma : v);
}

inline float Lerp(float a, float b, float alpha) {
  float d = a + ((b - a) * alpha);
  return d;
}

/// @}

}  // namespace arctic

#endif  // ENGINE_SCALAR_MATH_H_
