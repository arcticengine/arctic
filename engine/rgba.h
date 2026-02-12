// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2017 - 2020 Huldra
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

#ifndef ENGINE_RGBA_H_
#define ENGINE_RGBA_H_

#include "engine/arctic_types.h"

#define MASK_LO    0x00ff00fful
#define MASK_HI    0xff00ff00ul
#define MASK_BLEND 0xfefefefful

namespace arctic {

/// @addtogroup global_color
/// @{

/// @brief Represents a color in RGBA format.
struct Rgba {
  union {
    struct {
      Ui8 r;  ///< Red component
      Ui8 g;  ///< Green component
      Ui8 b;  ///< Blue component
      Ui8 a;  ///< Alpha component
    };
    Ui32 rgba;  ///< Combined RGBA value
    Ui8 element[4];  ///< Array access to color components
  };

  /// @brief Default constructor.
  Rgba() {}

  /// @brief Constructor for RGB color (alpha set to 255).
  /// @param r_in Red component
  /// @param g_in Green component
  /// @param b_in Blue component
  explicit Rgba(Ui8 r_in, Ui8 g_in, Ui8 b_in) {
    r = r_in;
    g = g_in;
    b = b_in;
    a = 255;
  }

  /// @brief Constructor for RGBA color.
  /// @param r_in Red component
  /// @param g_in Green component
  /// @param b_in Blue component
  /// @param a_in Alpha component (255 is opaque, 0 is transparent)
  explicit Rgba(Ui8 r_in, Ui8 g_in, Ui8 b_in, Ui8 a_in) {
    r = r_in;
    g = g_in;
    b = b_in;
    a = a_in;
  }

  /// @brief Constructor from 32-bit RGBA value.
  /// @param rgba_in 32-bit value containing 0xAABBGGRR (e.g., 0xff00ff00 is opaque green)
  explicit Rgba(Ui32 rgba_in) {
    rgba = rgba_in;
  }

  /// @brief Array subscript operator.
  /// @param i Index of the color component (0-3)
  /// @return Reference to the color component
  Ui8 &operator[](Si32 i) {
    return element[i];
  }

  /// @brief Const array subscript operator.
  /// @param i Index of the color component (0-3)
  /// @return Const reference to the color component
  const Ui8 &operator[](Si32 i) const {
    return element[i];
  }

  /// @brief Assignment operator.
  /// @param v Rgba color to assign
  /// @return Reference to this object
  Rgba &operator =(const Rgba &v) {
    rgba = v.rgba;
    return *this;
  }

  /// @brief Equality comparison operator.
  /// @param v Rgba color to compare
  /// @return True if colors are equal, false otherwise
  const bool operator== (const Rgba &v) const {
    return rgba == v.rgba;
  }

  /// @brief Inequality comparison operator.
  /// @param v Rgba color to compare
  /// @return True if colors are not equal, false otherwise
  const bool operator!= (const Rgba &v) const {
    return rgba != v.rgba;
  }
};

/// @brief Linearly interpolate between two colors.
/// @param a First color
/// @param b Second color
/// @param f Interpolation factor (0.0 to 1.0)
/// @return Interpolated color
inline Rgba Mix(Rgba const &a, Rgba const &b, float const f) {
  return Rgba(static_cast<Ui8>(a.r * (1.0f - f) + f * b.r),
    static_cast<Ui8>(a.g * (1.0f - f) + f * b.g),
    static_cast<Ui8>(a.b * (1.0f - f) + f * b.b),
    static_cast<Ui8>(a.a * (1.0f - f) + f * b.a));
}

/// @brief Get the minimum of two colors component-wise.
/// @param a First color
/// @param b Second color
/// @return Color with minimum components
inline Rgba Min(const Rgba a, const Rgba b) {
  return Rgba((a.r < b.r) ? a.r : b.r,
              (a.g < b.g) ? a.g : b.g,
              (a.b < b.b) ? a.b : b.b,
              (a.a < b.a) ? a.a : b.a);
}

/// @brief Get the maximum of two colors component-wise.
/// @param a First color
/// @param b Second color
/// @return Color with maximum components
inline Rgba Max(const Rgba a, const Rgba b) {
  return Rgba((a.r > b.r) ? a.r : b.r,
              (a.g > b.g) ? a.g : b.g,
              (a.b > b.b) ? a.b : b.b,
              (a.a > b.a) ? a.a : b.a);
}

/// @brief Clamp color components between two colors.
/// @param rgba Color to clamp
/// @param mi Minimum color
/// @param ma Maximum color
/// @return Clamped color
inline Rgba Clamp(const Rgba rgba, const Rgba mi, const Rgba ma) {
  return Max(Min(rgba, ma), mi);
}

/// @brief Fast blend of two colors.
/// @param c1 First color
/// @param c2 Second color
/// @return Blended color
inline Rgba BlendFast(Rgba c1, Rgba c2) {
  return Rgba(static_cast<Ui32>((
      (static_cast<Ui64>(c1.rgba) & MASK_BLEND) +
      (static_cast<Ui64>(c2.rgba) & MASK_BLEND)) >> 1u));
}

/// @brief Mix two colors with a given alpha value.
/// @param c1 First color
/// @param c2 Second color
/// @param alpha_1_8 Alpha value (0-255)
/// @return Mixed color
inline Rgba Mix(Rgba c1, Rgba c2, Ui32 alpha_1_8) {
  Ui32 a = c1.rgba & MASK_LO;
  Ui32 b = c2.rgba & MASK_LO;
  Ui32 d = a + (((b - a) * alpha_1_8) >> 8u);
  d = d & MASK_LO;

  Ui32 m = (c1.rgba & MASK_HI) >> 8u;
  Ui32 n = (c2.rgba & MASK_HI) >> 8u;
  Ui32 e = (c1.rgba & MASK_HI) + ((n - m) * alpha_1_8);
  e = e & MASK_HI;

  return Rgba(d | e);
}

/// @brief Scale color by an alpha value.
/// @param c Color to scale
/// @param alpha_1_8 Alpha value (0-255)
/// @return Scaled color
inline Rgba Scale(Rgba c, Ui32 alpha_1_8) {
  Ui32 a = c.rgba & MASK_LO;
  Ui32 d = (a * alpha_1_8) >> 8u;
  d = d & MASK_LO;

  a = (c.rgba & MASK_HI) >> 8u;
  Ui32 e = a * alpha_1_8;
  e = e & MASK_HI;

  return Rgba(d | e);
}

/// @brief Linear interpolation between two colors.
/// @param c1 First color
/// @param c2 Second color
/// @param alpha_1_8 Interpolation factor (0-255)
/// @return Interpolated color
inline Rgba Lerp(Rgba c1, Rgba c2, Si32 alpha_1_8) {
  Ui32 a = c1.rgba & MASK_LO;
  Ui32 b = c2.rgba & MASK_LO;
  Ui32 d = a + (((b - a) * static_cast<Ui32>(alpha_1_8)) >> 8u);
  d = d & MASK_LO;

  a = (c1.rgba & MASK_HI) >> 8u;
  b = (c2.rgba & MASK_HI) >> 8u;
  Ui32 e = (c1.rgba & MASK_HI) + ((b - a) * static_cast<Ui32>(alpha_1_8));
  e = e & MASK_HI;

  return Rgba(d | e);
}

/// @brief Convert color to grayscale.
/// @param c Color to convert
/// @return Grayscale color
inline Rgba GetGray(Rgba c) {
  Ui32 res = 19595 * static_cast<Ui32>(c.r)
    + 38470 * static_cast<Ui32>(c.g)
    + 7471 * static_cast<Ui32>(c.b);
  Ui8 x = (Ui8)(res >> 16);
  return Rgba(x, x, x, c.a);
}

/// @brief Interpolate color in a bi-linear way.
/// @details Bilerp calculates the color at point P:
/// @code
///    ^y
/// 256+ c      d
///    |
///  ay+.....P
///    |     .
///   0+ a   .  b
///    +-+---+--+-->x
///      0   ax 256
/// @endcode
/// @param a Bottom-left color
/// @param b Bottom-right color
/// @param c Top-left color
/// @param d Top-right color
/// @param ax X-axis interpolation factor (0-255), 0 = left, 255 = right
/// @param ay Y-axis interpolation factor (0-255), 0 = bottom, 255 = top
/// @return Interpolated color
inline Rgba Bilerp(Rgba a, Rgba b, Rgba c, Rgba d, Si32 ax, Si32 ay) {
  const Si32 axy = (ax * ay) >> 8u;
  Ui32 aa = a.rgba & MASK_LO;
  Ui32 bb = b.rgba & MASK_LO;
  Ui32 cc = c.rgba & MASK_LO;
  Ui32 dd = d.rgba & MASK_LO;

  Ui32 rb = (aa + (((bb - aa) * static_cast<Ui32>(ax)
          + (cc - aa) * static_cast<Ui32>(ay)
          + (aa + dd - bb - cc) * static_cast<Ui32>(axy)) >> 8u))
    & MASK_LO;

  aa = (a.rgba & MASK_HI) >> 8u;
  bb = (b.rgba & MASK_HI) >> 8u;
  cc = (c.rgba & MASK_HI) >> 8u;
  dd = (d.rgba & MASK_HI) >> 8u;

  Ui32 gg = ((a.rgba & MASK_HI) + (
      (bb - aa) * static_cast<Ui32>(ax)
      + (cc - aa) * static_cast<Ui32>(ay)
      + (aa + dd - bb - cc) * static_cast<Ui32>(axy)))
    & MASK_HI;

  return Rgba(rb | gg);
}

/// @}

}  // namespace arctic

#undef MASK_BLEND
#undef MASK_HI
#undef MASK_LO

#endif  // ENGINE_RGBA_H_
