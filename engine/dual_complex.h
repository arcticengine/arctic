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

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "engine/vec2f.h"

namespace arctic {

template<class T>
class DualComplex {
public:
  T real_x;
  T real_y;
  T dual_x;
  T dual_y;

  /// @brief constructor
  DualComplex() {
    real_x = 1;
    real_y = 0;
    dual_x = 0;
    dual_y = 0;
  }

  /// @brief Constructor that creates DualComplex with given real and dual parts
  /// @param a_real_x the x-coordinate of the real part
  /// @param a_real_y the y-coordinate of the real part
  /// @param a_dual_x the x-coordinate of the dual part
  /// @param a_dual_y the y-coordinate of the dual part
  DualComplex(T a_real_x, T a_real_y, T a_dual_x, T a_dual_y) {
    real_x = a_real_x;
    real_y = a_real_y;
    dual_x = a_dual_x;
    dual_y = a_dual_y;
  }

  /// @brief Constructor that creates DualComplex which represents the rotation around a given point
  /// @param x the x-coordinate of the center of the rotation
  /// @param y the y-coordinate of the center of the rotation
  /// @param theta the degree in radian of the rotation
  DualComplex(T x, T y, T theta) {
//        DualComplex v(cos(theta/2.0),sin(theta/2.0),0,0);
//        DualComplex u(1,0,-x/2.0,-y/2.0);
//        DualComplex w(1,0,x/2.0,y/2.0);
//        DualComplex result = w*v*u;
    real_x = std::cos(theta * 0.5f);
    real_y = std::sin(theta * 0.5f);
    dual_x = std::sin(theta * 0.5f) * y;
    dual_y = -std::sin(theta * 0.5f) * x;
  }

  DualComplex(Vec2F p, T theta) {
//        DualComplex v(cos(theta/2.0),sin(theta/2.0),0,0);
//        DualComplex u(1,0,-x/2.0,-y/2.0);
//        DualComplex w(1,0,x/2.0,y/2.0);
//        DualComplex result = w*v*u;
    real_x = std::cos(theta * 0.5f);
    real_y = std::sin(theta * 0.5f);
    dual_x = std::sin(theta * 0.5f) * p.y;
    dual_y = -std::sin(theta * 0.5f) * p.x;
  }

  DualComplex(T x, T y) {
    real_x = 1;
    real_y = 0;
    dual_x = x * 0.5f;
    dual_y = y * 0.5f;
  }

  T GetAngle() const {
    return std::acos(real_x) * (real_y >= (T)0 ? 2.f : -2.f);
  }

  /// @brief Conjugation
  /// @return the conjugate of the DualComplex
  DualComplex Conj() const {
    return DualComplex(real_x, -real_y, dual_x, dual_y);
  }

  Vec2F Transform(Vec2F vec) const {
    return Vec2F(
        static_cast<float>(
          (real_x * real_x - real_y * real_y) * static_cast<T>(vec.x)
          + 2 * (real_x * dual_x - real_y * dual_y -
            real_x * real_y * static_cast<T>(vec.y))),
        static_cast<float>(
          (real_x * real_x - real_y * real_y) * static_cast<T>(vec.y)
          + 2 * (real_x * real_y * static_cast<T>(vec.x) + real_x * dual_y +
            real_y * dual_x)));
  }

  /// @brief normalisation to unit length DualComplex
  /// @return DualComplex the unit length DualComplex
  DualComplex Normalised() const {
    T norm = std::sqrt(real_x * real_x + real_y * real_y);
    return DualComplex(real_x / norm, real_y / norm,
      dual_x / norm, dual_y / norm);
  }

  DualComplex TranslationScaled(T scale) const {
    return DualComplex(real_x, real_y,
      dual_x * scale, dual_y * scale);
  }

  /// @brief norm
  /// @return norm of the DualComplex
  T Norm() const {
    return (std::sqrt(real_x * real_x + real_y * real_y));
  }

  /// @brief multiplication by a scalar
  DualComplex& operator*=(T scale) {
    real_x *= scale;
    real_y *= scale;
    dual_x *= scale;
    dual_y *= scale;
    return *this;
  }

  /// @brief sum
  DualComplex& operator+=(DualComplex toSum) {
    real_x += toSum.real_x;
    real_y += toSum.real_y;
    dual_x += toSum.dual_x;
    dual_y += toSum.dual_y;
    return *this;
  }

  /// @brief multiplication by a scalar
  DualComplex operator*(T scale) const {
    return DualComplex(real_x * scale, real_y * scale,
      dual_x * scale, dual_y * scale);
  }

  /// @brief substitution
  DualComplex operator=(DualComplex dcn) {
    real_x = dcn.real_x;
    real_y = dcn.real_y;
    dual_x = dcn.dual_x;
    dual_y = dcn.dual_y;
    return *this;
  }

  /// @brief multiplication by a DualComplex
  DualComplex operator*(DualComplex dcn) const {
    return DualComplex(
      real_x * dcn.real_x - real_y * dcn.real_y,
      real_x * dcn.real_y + real_y * dcn.real_x,
      real_x * dcn.dual_x - real_y * dcn.dual_y + dual_x * dcn.real_x +
        dual_y * dcn.real_y,
      real_x * dcn.dual_y + real_y * dcn.dual_x - dual_x * dcn.real_y +
        dual_y * dcn.real_x);
  }

  /// @brief sum
  DualComplex operator+(DualComplex dcn) const {
    return DualComplex(real_x + dcn.real_x, real_y + dcn.real_y,
      dual_x + dcn.dual_x, dual_y + dcn.dual_y);
  }

  /// @brief linear blend (DLB)
  /// @param dcns array of DualComplex's to be blended
  /// @param weights weights of the correspoding DualComplex's
  /// @return blended normalised DualComplex
  static DualComplex Blend(std::vector<DualComplex> dcns, std::vector<T> weights) {
    assert(dcns.size() == weights.size());
    DualComplex result(0,0,0,0);
    for (size_t i = 0; i < dcns.size(); i++) {
      result += dcns[i] * weights[i];
    }
    return result.Normalised();
  }

  static DualComplex Lerp(DualComplex a, DualComplex b, T t) {
    return (a * ((T)1 - t) + b * t).Normalised();
  }

  static DualComplex Clerp(DualComplex a, DualComplex b, T t) {
    if (a.real_x * b.real_x + a.real_y * b.real_y < (T)0) {
      return (a * ((T)1 - t) + b * -t).Normalised();
    }
    return (a * ((T)1 - t) + b * t).Normalised();
  }

  static DualComplex Slerp(DualComplex a, DualComplex b, T t) {
    T cos_theta = a.real_x * b.real_x + a.real_y * b.real_y;
    if (cos_theta < (T) 0) {
      cos_theta = -cos_theta;
      b *= -1;
    }
    T scale_a;
    T scale_b;
    if ((1 - cos_theta) > 0.001) {
      T theta = std::acos(cos_theta);
      T sin_theta = std::sin(theta);
      scale_a = std::sin((1 - t) * theta) / sin_theta;
      scale_b = std::sin(t * theta) / sin_theta;
    } else {
      scale_a = 1 - t;
      scale_b = t;
    }
    return DualComplex(
      scale_a * a.real_x + scale_b * b.real_x,
      scale_a * a.real_y + scale_b * b.real_y,
      (1-t) * a.dual_x + t*b.dual_x,
      (1-t) * a.dual_y + t*b.dual_y).Normalised();
  }
};

template<class T>
std::ostream& operator<<(std::ostream &os, const DualComplex<T> &dt) {
  os << dt.real_x << "+" << dt.real_y <<
     " i + (" << dt.dual_x << "+" << dt.dual_y << "i)e";
  return os;
}

extern template class DualComplex<float>;
extern template class DualComplex<double>;

typedef DualComplex<float> DualComplexF;
typedef DualComplex<double> DualComplexD;

extern template std::ostream& operator<<(std::ostream &os, const DualComplexF &dt);
extern template std::ostream& operator<<(std::ostream &os, const DualComplexD &dt);

}  // namespace arctic

extern template class std::vector<arctic::DualComplexF>;
extern template class std::vector<arctic::DualComplexD>;

#endif  // ENGINE_DUAL_COMPLEX_H_
