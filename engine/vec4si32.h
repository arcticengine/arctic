// The MIT License(MIT)
//
// Copyright 2015 - 2016 Inigo Quilez
// Copyright 2016 Huldra
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

#ifndef ENGINE_VEC4SI32_H_
#define ENGINE_VEC4SI32_H_

#include <cmath>
#include "engine/arctic_types.h"

namespace arctic {

struct Vec4Si32 {
  union {
    struct {
      Si32 x;
      Si32 y;
      Si32 z;
      Si32 w;
    };
    Si32 element[4];
  };

  Vec4Si32() {}

  explicit Vec4Si32(Si32 a, Si32 b, Si32 c, Si32 d) {
    x = a;
    y = b;
    z = c;
    w = d;
  }
  explicit Vec4Si32(Si32 s) {
    x = s;
    y = s;
    z = s;
    w = s;
  }

  Si32 &operator[](Si32 i) {
    return element[i];
  }
  const Si32 &operator[](Si32 i) const {
    return element[i];
  }
};


}  // namespace arctic

#endif  // ENGINE_VEC4SI32_H_
