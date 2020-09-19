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
struct Rgb {
  union {
    struct {
      Ui8 r;
      Ui8 g;
      Ui8 b;
    };
    Ui8 element[3];
  };

  Rgb() {}

  explicit Rgb(Ui8 r_in, Ui8 g_in, Ui8 b_in) {
    r = r_in;
    g = g_in;
    b = b_in;
  }
  explicit Rgb(Ui8 s) {
    r = s;
    g = s;
    b = s;
  }
  // Byte order is 0xbbggrr
  explicit Rgb(Ui32 rgb_in) {
    r = static_cast<Ui8>(rgb_in & 0xfful);
    g = static_cast<Ui8>((rgb_in >> 8ul) & 0xfful);
    b = static_cast<Ui8>((rgb_in >> 16ul) & 0xfful);
  }
  Ui8 &operator[](Si32 i) {
    return element[i];
  }
  const Ui8 &operator[](Si32 i) const {
    return element[i];
  }

  Rgb &operator =(const Rgb &v) {
    r = v.r;
    g = v.g;
    b = v.b;
    return *this;
  }

  const bool operator== (const Rgb &v) const {
    return r == v.r && g == v.g && b == v.b;
  }
  const bool operator!= (const Rgb &v) const {
    return r != v.r || g != v.g || b != v.b;
  }
};
/// @}

}  // namespace arctic

#endif  // ENGINE_RGB_H_
