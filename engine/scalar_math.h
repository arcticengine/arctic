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

#define _USE_MATH_DEFINES
#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_math
/// @{

/// @brief Clamps an Si32 value between a minimum and maximum value.
/// @param v The value to clamp.
/// @param mi The minimum allowed value.
/// @param ma The maximum allowed value.
/// @return The clamped value.
inline Si32 Clamp(const Si32 v, const Si32 mi, const Si32 ma) {
  return (v < mi) ? mi : ((v > ma) ? ma : v);
}

/// @brief Clamps a float value between a minimum and maximum value.
/// @param v The value to clamp.
/// @param mi The minimum allowed value.
/// @param ma The maximum allowed value.
/// @return The clamped value.
inline float Clamp(const float v, const float mi, const float ma) {
    return (v < mi) ? mi : ((v > ma) ? ma : v);
}

/// @brief Clamps a double value between a minimum and maximum value.
/// @param v The value to clamp.
/// @param mi The minimum allowed value.
/// @param ma The maximum allowed value.
/// @return The clamped value.
inline double Clamp(const double v, const double mi, const double ma) {
    return (v < mi) ? mi : ((v > ma) ? ma : v);
}

/// @brief Performs linear interpolation between two float values.
/// @param a The starting value.
/// @param b The ending value.
/// @param alpha The interpolation factor (0.0 to 1.0).
/// @return The interpolated value.
inline double Lerp(const double a, const double b, const double alpha) {
  const double d = a + ((b - a) * alpha);
  return d;
}

/// @brief Performs linear interpolation between two float values.
/// @param a The starting value.
/// @param b The ending value.
/// @param alpha The interpolation factor (0.0 to 1.0).
/// @return The interpolated value.
inline float Lerp(const float a, const float b, const float alpha) {
  const float d = a + ((b - a) * alpha);
  return d;
}

/// @}

}  // namespace arctic

#endif  // ENGINE_SCALAR_MATH_H_
