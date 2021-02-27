// The MIT License (MIT)
//
// Copyright (c) 2021 Huldra
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

#ifndef ENGINE_ARRAY2_H_
#define ENGINE_ARRAY2_H_

#include <vector>
#include "engine/arctic_types.h"
#include "engine/vec3si32.h"

namespace arctic {

template<class T> class Array2 {
  Vec2Si32 size_;
  std::vector<T> data_;
 public:
  explicit Array2<T>()
    : size_(0, 0) {
  }
  explicit Array2(const Si32 width, const Si32 height)
    : size_(width, height)
    , data_(width*height) {
  }
  explicit Array2(const Vec2Si32 size)
    : size_(size)
    , data_(size_.x*size_.y) {
  }
  explicit Array2(const Array2<T> &original)
    : size_(original.size_)
    , data_(original.data_) {
  }
  explicit Array2(Array2 &&original)
    : size_(original.size_)
    , data_(std::move(original.data_)) {
  }
  Array2<T>& operator=(const Array2<T> &original) {
    size_ = original.size_;
    data_ = original.data_;
    return *this;
  }
  T& At(const Vec2Si32 pos) {
    return data_[pos.x + pos.y * size_.x];
  }
  const T& At(const Vec2Si32 pos) const {
    return data_[pos.x + pos.y * size_.x];
  }
  T& At(const Si32 x, const Si32 y) {
    return data_[x + y * size_.x];
  }
  const T& At(const Si32 x, const Si32 y) const {
    return data_[x + y * size_.x];
  }
  Vec2Si32 Size() const {
    return size_;
  }
};

}  // namespace arctic

#endif  // ENGINE_ARRAY2_H_
