// The MIT License (MIT)
//
// Copyright (c) 2020 Huldra
// Copyright (c) 2014 Shizuo KAJI <shizuo.kaji@gmail.com>
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

// Library for Anti-commutative Dual Complex Numbers
// For the detail, look at
// G. Matsuda, S. Kaji, and H. Ochiai, Anti-commutative Dual Complex Numbers and 2D Rigid Transformation,
//Mathematical Progress in Expressive Image Synthesis I, Springer-Japan, 2014.
// http://arxiv.org/abs/1601.01754

#ifndef ENGINE_DUAL_COMPLEX_H_
#define ENGINE_DUAL_COMPLEX_H_

#include <math.h>
#include <vector>

namespace arctic {

template<class T>
class DualComplex {
public:
  /// coordinates which is the real part of the DualComplex
  T real[2];
  /// coordinates which is the complex part of the DualComplex
  T dual[2];

  /// constructor
  DualComplex() {
    real[0] = 0;
    real[1] = 0;
    dual[0] = 0;
    dual[1] = 0;
  }

  /** Constructor that creats DualComplex with given real and dual parts
   @param a_real0 the x-coordinate of the real part
   @param a_real1 the y-coordinate of the real part
   @param a_dual0 the x-coordinate of the dual part
   @param a_dual1 the y-coordinate of the dual part
   */
  DualComplex(T a_real0, T a_real1, T a_dual0, T a_dual1) {
    real[0] = a_real0;
    real[1] = a_real1;
    dual[0] = a_dual0;
    dual[1] = a_dual1;
  }

  /** Constructor that creats DualComplex which represents the rotation around a given point
   @param x the x-coordinate of the center of the rotation
   @param x the y-coordinate of the center of the rotation
   @param theta the degree in radian of the rotation
   */
  DualComplex(T x, T y, T theta) {
//        DualComplex v(cos(theta/2.0),sin(theta/2.0),0,0);
//        DualComplex u(1,0,-x/2.0,-y/2.0);
//        DualComplex w(1,0,x/2.0,y/2.0);
//        DualComplex result = w*v*u;
    real[0] = cos(theta / 2.0);
    real[1] = sin(theta / 2.0);
    dual[0] = sin(theta / 2.0) * y;
    dual[1] = -sin(theta / 2.0) * x;
  }

  /// conjugation
  /// @return the conjugate of the DualComplex
  DualComplex Conj() {
    DualComplex result;
    result.real[0] = real[0];
    result.real[1] = -real[1];
    result.dual[0] = dual[0];
    result.dual[1] = dual[1];
    return result;
  }

  /** action
   @param dcn DualComplex which acts
   @return the resulting DualComplex acted by dcn
  */
  DualComplex ActedBy(DualComplex dcn) {
    DualComplex result;
    result.real[0] = 1;
    result.real[1] = 0;
    result.dual[0] =
      (dcn.real[0] * dcn.real[0] - dcn.real[1] * dcn.real[1]) * dual[0]
        + 2 * (dcn.real[0] * dcn.dual[0] - dcn.real[1] * dcn.dual[1] -
        dcn.real[0] * dcn.real[1] * dual[1]);
    result.dual[1] =
      (dcn.real[0] * dcn.real[0] - dcn.real[1] * dcn.real[1]) * dual[1]
        + 2 * (dcn.real[0] * dcn.real[1] * dual[0] + dcn.real[0] * dcn.dual[1] +
        dcn.real[1] * dcn.dual[0]);
    return result;
  }

  /** normalisation to unit length DualComplex
   @return DualComplex the unit length DualComplex
   */
  DualComplex Normalised() {
    DualComplex result;
    T norm = sqrt(real[0] * real[0] + real[1] * real[1]);
    result.real[0] = real[0] / norm;
    result.real[1] = real[1] / norm;
    result.dual[0] = dual[0] / norm;
    result.dual[1] = dual[1] / norm;
    return result;
  }

  /** norm
   @return norm of the DualComplex
   */
  T Norm() {
    return (sqrt(real[0] * real[0] + real[1] * real[1]));
  }

  /// multiplication by a scalar
  void operator*=(T scale) {
    real[0] *= scale;
    real[1] *= scale;
    dual[0] *= scale;
    dual[1] *= scale;
    return *this;
  }

  /// sum
  void operator+=(DualComplex toSum) {
    real[0] += toSum.real[0];
    real[1] += toSum.real[1];
    dual[0] += toSum.dual[0];
    dual[1] += toSum.dual[1];
  }

  /// multiplication by a scalar
  DualComplex operator*(T scale) {
    DualComplex result;
    result.real[0] = real[0] * scale;
    result.real[1] = real[1] * scale;
    result.dual[0] = dual[0] * scale;
    result.dual[1] = dual[1] * scale;
    return result;
  }

  /// substitution
  void operator=(DualComplex dcn) {
    real[0] = dcn.real[0];
    real[1] = dcn.real[1];
    dual[0] = dcn.dual[0];
    dual[1] = dcn.dual[1];
  }

  /// multiplication by a DualComplex
  DualComplex operator*(DualComplex dcn) {
    DualComplex result;
    result.real[0] = real[0] * dcn.real[0] - real[1] * dcn.real[1];
    result.real[1] = real[0] * dcn.real[1] + real[1] * dcn.real[0];
    result.dual[0] =
      real[0] * dcn.dual[0] - real[1] * dcn.dual[1] + dual[0] * dcn.real[0] +
        dual[1] * dcn.real[1];
    result.dual[1] =
      real[0] * dcn.dual[1] + real[1] * dcn.dual[0] - dual[0] * dcn.real[1] +
        dual[1] * dcn.real[0];
    return result;
  }

  /// sum
  DualComplex operator+(DualComplex dcn) {
    DualComplex result;
    result.real[0] = real[0] + dcn.real[0];
    result.real[1] = real[1] + dcn.real[1];
    result.dual[0] = dual[0] + dcn.dual[0];
    result.dual[1] = dual[1] + dcn.dual[1];
    return result;
  }

  /** linear blend (DLB)
   @param dcns array of DualComplex's to be blended
   @param weights weights of the correspoding DualComplex's
   @return blended normalised DualComplex
   */
  DualComplex Blend(std::vector<DualComplex> dcns, std::vector<T> weights) {
    assert(dcns.size() == weights.size());
    DualComplex result;
    for (int i = 0; i < dcns.size(); i++) {
      result += dcns[i] * weights[i];
    }
    return (result.Normalised());
  }
};

template<class T>
std::ostream &operator<<(std::ostream &os, const DualComplex<T> &dt) {
  os << dt.real[0] << "+" << dt.real[1] <<
     " i + (" << dt.dual[0] << "+" << dt.dual[1] << "i)e";
  return os;
}

extern template class DualComplex<float>;
extern template class DualComplex<double>;

typedef DualComplex<float> DualComplexF;
typedef DualComplex<double> DualComplexD;

}  // namespace arctic

extern template class std::vector<arctic::DualComplexF>;
extern template class std::vector<arctic::DualComplexD>;

#endif  // ENGINE_DUAL_COMPLEX_H_
