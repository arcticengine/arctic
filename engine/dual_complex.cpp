// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
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

#include "engine/dual_complex.h"

#include <iostream>

namespace arctic {

template class DualComplex<float>;
template class DualComplex<double>;

std::ostream& operator<<(std::ostream &os, const DualComplexF &dt) {
  os << dt.real_x << "+" << dt.real_y <<
     " i + (" << dt.dual_x << "+" << dt.dual_y << "i)e";
  return os;
}
std::ostream& operator<<(std::ostream &os, const DualComplexD &dt) {
  os << dt.real_x << "+" << dt.real_y <<
     " i + (" << dt.dual_x << "+" << dt.dual_y << "i)e";
  return os;
}


}  // namespace arctic

template class std::vector<arctic::DualComplexF>;
template class std::vector<arctic::DualComplexD>;
