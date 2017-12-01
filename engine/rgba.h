// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

namespace arctic {

struct Rgba {
  union {
    struct {
      Ui8 r;
      Ui8 g;
      Ui8 b;
      Ui8 a;
    };
    Ui32 rgba;
    Ui8 element[4];
  };

  Rgba() {}

  explicit Rgba(Ui8 r_in, Ui8 g_in, Ui8 b_in) {
    r = r_in;
    g = g_in;
    b = b_in;
    a = 255;
  }
  explicit Rgba(Ui8 r_in, Ui8 g_in, Ui8 b_in, Ui8 a_in) {
    r = r_in;
    g = g_in;
    b = b_in;
    a = a_in;
  }
  explicit Rgba(Ui8 s) {
    r = s;
    g = s;
    b = s;
    a = 255;
  }
  explicit Rgba(Ui32 rgba_in) {
    rgba = rgba_in;
  }
  Ui8 &operator[](Si32 i) {
    return element[i];
  }
  const Ui8 &operator[](Si32 i) const {
    return element[i];
  }

  Rgba &operator =(const Rgba &v) {
    rgba = v.rgba;
    return *this;
  }

  const bool operator== (const Rgba &v) const {
    return rgba == v.rgba;
  }
  const bool operator!= (const Rgba &v) const {
    return rgba != v.rgba;
  }
};

}  // namespace arctic

#endif  // ENGINE_RGBA_H_
