// The MIT License (MIT)
//
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

#ifndef ENGINE_RGB_H_
#define ENGINE_RGB_H_

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_color
/// @{

/// @brief Represents an RGB color.
struct Rgb {
  union {
    struct {
      Ui8 r;  ///< Red component
      Ui8 g;  ///< Green component
      Ui8 b;  ///< Blue component
    };
    Ui8 element[3];  ///< Array access to color components
  };

  /// @brief Default constructor.
  Rgb() {}

  /// @brief Constructs an RGB color from individual components.
  /// @param r_in Red component
  /// @param g_in Green component
  /// @param b_in Blue component
  explicit Rgb(Ui8 r_in, Ui8 g_in, Ui8 b_in) {
    r = r_in;
    g = g_in;
    b = b_in;
  }

  /// @brief Constructs a grayscale RGB color.
  /// @param s Intensity value for all components
  explicit Rgb(Ui8 s) {
    r = s;
    g = s;
    b = s;
  }

  /// @brief Constructs an RGB color from a 32-bit integer.
  /// @param rgb_in 32-bit integer in 0xbbggrr format
  explicit Rgb(Ui32 rgb_in) {
    r = static_cast<Ui8>(rgb_in & 0xfful);
    g = static_cast<Ui8>((rgb_in >> 8ul) & 0xfful);
    b = static_cast<Ui8>((rgb_in >> 16ul) & 0xfful);
  }

  /// @brief Array subscript operator for accessing color components.
  /// @param i Index of the component (0 for red, 1 for green, 2 for blue)
  /// @return Reference to the color component
  Ui8 &operator[](Si32 i) {
    return element[i];
  }

  /// @brief Const array subscript operator for accessing color components.
  /// @param i Index of the component (0 for red, 1 for green, 2 for blue)
  /// @return Const reference to the color component
  const Ui8 &operator[](Si32 i) const {
    return element[i];
  }

  /// @brief Assignment operator.
  /// @param v RGB color to assign from
  /// @return Reference to this object
  Rgb &operator =(const Rgb &v) {
    r = v.r;
    g = v.g;
    b = v.b;
    return *this;
  }

  /// @brief Equality comparison operator.
  /// @param v RGB color to compare with
  /// @return True if colors are equal, false otherwise
  const bool operator== (const Rgb &v) const {
    return r == v.r && g == v.g && b == v.b;
  }

  /// @brief Inequality comparison operator.
  /// @param v RGB color to compare with
  /// @return True if colors are not equal, false otherwise
  const bool operator!= (const Rgb &v) const {
    return r != v.r || g != v.g || b != v.b;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_RGB_H_
