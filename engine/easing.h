// The MIT License (MIT)
//
// Copyright (c) 2001 Robert Penner
// Copyright (c) 2019 Juan Carlos
// Copyright (c) 2020 Huldra
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

#ifndef ENGINE_EASING_H_
#define ENGINE_EASING_H_

#define _USE_MATH_DEFINES
#include <cmath>

namespace arctic {

#define INTERNAL_PI (3.141592653589793238463f)

namespace easings_internal {
inline float Linear(float t, float b, float c, float d) {
  return c * t / d + b;
}

inline float InCubic(float t, float b, float c, float d) {
  t /= d;
  return c * t * t * t + b;
}

inline float OutCubic(float t, float b, float c, float d) {
  t = t / d - 1.f;
  return c * (t * t * t + 1.f) + b;
}

inline float InOutCubic(float t, float b, float c, float d) {
  t /= d / 2.f;
  if (t < 1.f) {
    return c / 2.f * t * t * t + b;
  }
  t -= 2.f;
  return c / 2.f * (t * t * t + 2.f) + b;
}

inline float InQuad(float t, float b, float c, float d) {
  t /= d;
  return c * t * t + b;
}

inline float OutQuad(float t, float b, float c, float d) {
  t /= d;
  return -c * t * (t - 2.f) + b;
}

inline float InOutQuad(float t, float b, float c, float d) {
  t /= d / 2.f;
  if (t < 1.f) {
    return c / 2.f * t * t + b;
  }
  --t;
  return -c / 2.f * (t * (t - 2.f) - 1.f) + b;
}

inline float InQuart(float t, float b, float c, float d) {
  t /= d;
  return c * t * t * t * t + b;
}

inline float OutQuart(float t, float b, float c, float d) {
  t = t / d - 1.f;
  return -c * (t * t * t * t - 1.f) + b;
}

inline float InOutQuart(float t, float b, float c, float d) {
  t /= d / 2.f;
  if (t < 1.f) {
    return c / 2.f * t * t * t * t + b;
  }
  t -= 2.f;
  return -c / 2.f * (t * t * t * t - 2.f) + b;
}

inline float InQuint(float t, float b, float c, float d) {
  t /= d;
  return c * t * t * t * t * t + b;
}

inline float OutQuint(float t, float b, float c, float d) {
  t = t / d - 1.f;
  return c * (t * t * t * t * t + 1.f) + b;
}

inline float InOutQuint(float t, float b, float c, float d) {
  t /= d / 2.f;
  if (t < 1.f) {
    return c / 2.f * t * t * t * t * t + b;
  }
  t -= 2.f;
  return c / 2.f * (t * t * t * t * t + 2.f) + b;
}

inline float InSine(float t, float b, float c, float d) {
  return -c * cosf(t / d * (INTERNAL_PI / 2.f)) + c + b;
}

inline float OutSine(float t, float b, float c, float d) {
  return c * sinf(t / d * (INTERNAL_PI / 2.f)) + b;
}

inline float InOutSine(float t, float b, float c, float d) {
  return -c / 2.f * (cosf(INTERNAL_PI * t / d) - 1.f) + b;
}

inline float InOutExpo(float t, float b, float c, float d) {
  t /= d / 2.f;
  if (t < 1.f) {
    return c / 2.f * powf(2.f, 10.f * (t - 1.f)) + b;
  }
  return c / 2.f * (-powf(2.f, -10.f * --t) + 2.f) + b;
}

inline float InCirc(float t, float b, float c, float d) {
  t /= d;
  return -c * (sqrtf(1.f - t * t) - 1.f) + b;
}

inline float OutCirc(float t, float b, float c, float d) {
  t = t / d - 1.f;
  return c * sqrtf(1.f - t * t) + b;
}

inline float InOutCirc(float t, float b, float c, float d) {
  t /= d / 2.f;
  if (t < 1.f) {
    return -c / 2.f * (sqrtf(1.f - t * t) - 1.f) + b;
  }
  t -= 2.f;
  return c / 2.f * (sqrtf(1.f - t * t) + 1.f) + b;
}

inline float InElastic(float t, float b, float c, float d) {
  float s = 1.70158f;
  float a = c;
  float p = d * 0.3f;
  if (a < fabsf(c)) {
    a = c;
    s = p / 4.f;
  } else {
    s = p / (2.f * INTERNAL_PI) * asinf(c / a);
  }
  t -= 1.f;
  return -(a * powf(2.f, 10.f * t) *
    sinf((t * d - s) * (2.f * INTERNAL_PI) / p)) + b;
}

inline float OutElastic(float t, float b, float c, float d) {
  float s = 1.70158f;
  float a = c;
  float p = d * 0.3f;

  if (a < fabsf(c)) {
    a = c;
    s = p / 4.f;
  } else {
    s = p / (2.f * INTERNAL_PI) * asinf(c / a);
  }
  return a * powf(2.f, -10.f * t) *
    sinf((t * d - s) * (2.f * INTERNAL_PI) / p) + c + b;
}

inline float InOutElastic(float t, float b, float c, float d) {
  float s = 1.70158f;
  float a = c;
  float p = d * (0.3f * 1.5f);
  if (a < fabsf(c)) {
    a = c;
    s = p / 4.f;
  } else {
    s = p / (2.f * INTERNAL_PI) * asinf(c / a);
  }
  if (t < 1.f) {
    t -= 1.f;
    return -0.5f * (a * powf(2.f, 10.f * t) *
      sinf((t * d - s) * (2.f * INTERNAL_PI) / p)) + b;
  }
  t -= 1.f;
  return a * powf(2.f, -10.f * t) *
    sinf((t * d - s) * (2.f * INTERNAL_PI) / p) * 0.5f + c + b;
}

inline float InBack(float t, float b, float c, float d) {
  float s = 1.70158f;
  t /= d;
  return c * t * t * ((s + 1.f) * t - s) + b;
}

inline float OutBack(float t, float b, float c, float d) {
  float s = 1.70158f;
  t = t / d - 1.f;
  return c * (t * t * ((s + 1.f) * t + s) + 1.f) + b;
}

inline float InOutBack(float t, float b, float c, float d) {
  float s = 1.70158f;
  t /= d / 2.f;
  s *= 1.525f;
  if (t < 1.f) {
    return c / 2.f * (t * t * ((s + 1.f) * t - s)) + b;
  }
  s *= 1.525f;
  t -= 2.f;
  return (c / 2.f * (t * t * (s + 1.f) * t + s) + 2.f) + b;
}


inline float OutBounce(float t, float b, float c, float d) {
  t /= d;
  if (t < 1.0f / 2.75f) {
    return c * (7.5625f * t * t) + b;
  }
  if (t < 2.0f / 2.75f) {
    t -= 1.5f / 2.75f;
    return c * (7.5625f * t * t + 0.75f) + b;
  }
  if (t < 2.5f / 2.75f) {
    t -= 2.25f / 2.75f;
    return c * (7.5625f * t * t + 0.9375f) + b;
  }
  t -= 2.625f / 2.75f;
  return c * (7.5625f * t * t + 0.984375f) + b;
}

inline float InBounce(float t, float b, float c, float d) {
  float v = OutBounce(d - t, 0.f, c, d);
  return c - v + b;
}

inline float InOutBounce(float t, float b, float c, float d) {
  float v;
  if (t < d / 2.f) {
    v = InBounce(t * 2.f, 0.f, c, d);
    return v * 0.5f + b;
  }
  v = OutBounce(t * 2.f - d, 0.f, c, d);
  return v * 0.5f + c * 0.5f + b;
}
}  // namespace easings_internal
#undef INTERNAL_PI

/// @addtogroup global_math
/// @{


inline float EaseLinear(float t) {
  return easings_internal::Linear(t, 0.f, 1.f, 1.f);
}

inline float EaseInCubic(float t) {
  return easings_internal::InCubic(t, 0.f, 1.f, 1.f);
}

inline float EaseOutCubic(float t) {
  return easings_internal::OutCubic(t, 0.f, 1.f, 1.f);
}

inline float EaseInOutCubic(float t) {
  return easings_internal::InOutCubic(t, 0.f, 1.f, 1.f);
}

inline float EaseInQuad(float t) {
  return easings_internal::InQuad(t, 0.f, 1.f, 1.f);
}

inline float EaseOutQuad(float t) {
  return easings_internal::OutQuad(t, 0.f, 1.f, 1.f);
}

inline float EaseInOutQuad(float t) {
  return easings_internal::InOutQuad(t, 0.f, 1.f, 1.f);
}

inline float EaseInElastic(float t) {
  return easings_internal::InElastic(t, 0.f, 1.f, 1.f);
}

inline float EaseOutElastic(float t) {
  return easings_internal::OutElastic(t, 0.f, 1.f, 1.f);
}

inline float EaseInOutElastic(float t) {
  return easings_internal::InOutElastic(t, 0.f, 1.f, 1.f);
}

inline float EaseInBounce(float t) {
  return easings_internal::InBounce(t, 0.f, 1.f, 1.f);
}

inline float EaseOutBounce(float t) {
  return easings_internal::OutBounce(t, 0.f, 1.f, 1.f);
}

inline float EaseInOutBounce(float t) {
  return easings_internal::InOutBounce(t, 0.f, 1.f, 1.f);
}

inline float EaseInBack(float t) {
  return easings_internal::InBack(t, 0.f, 1.f, 1.f);
}

inline float EaseOutBack(float t) {
  return easings_internal::OutBack(t, 0.f, 1.f, 1.f);
}

inline float EaseInOutBack(float t) {
  return easings_internal::InOutBack(t, 0.f, 1.f, 1.f);
}

/// @}

}  // namespace arctic

#endif  // ENGINE_EASING_H_
