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

#include "engine/vec2si32.h"
#include "engine/arctic_types.h"
#include <vector>

namespace arctic {

/// @brief A 2D array template class for storing elements of type T.
template<class T> class Array2 {
  Vec2Si32 size_;
  std::vector<T> data_;
 public:
  /// @brief Default constructor. Creates an empty Array2 with size (0, 0).
  explicit Array2<T>()
    : size_(0, 0) {
  }

  /// @brief Constructor that creates an Array2 with the specified width and height.
  /// @param width The width of the array.
  /// @param height The height of the array.
  explicit Array2(const Si32 width, const Si32 height)
    : size_(width, height)
    , data_(width*height) {
  }

  /// @brief Constructor that creates an Array2 with the specified size.
  /// @param size The size of the array as a Vec2Si32.
  explicit Array2(const Vec2Si32 size)
    : size_(size)
    , data_(size_.x*size_.y) {
  }

  /// @brief Copy constructor.
  /// @param original The Array2 to copy from.
  explicit Array2(const Array2<T> &original)
    : size_(original.size_)
    , data_(original.data_) {
  }

  /// @brief Move constructor.
  /// @param original The Array2 to move from.
  explicit Array2(Array2 &&original)
    : size_(original.size_)
    , data_(std::move(original.data_)) {
  }

  /// @brief Copy assignment operator.
  /// @param original The Array2 to copy from.
  /// @return A reference to the assigned Array2.
  Array2<T>& operator=(const Array2<T> &original) {
    size_ = original.size_;
    data_ = original.data_;
    return *this;
  }

  /// @brief Access an element at the specified position.
  /// @param pos The position as a Vec2Si32.
  /// @return A reference to the element at the specified position.
  T& At(const Vec2Si32 pos) {
    return data_[pos.x + pos.y * size_.x];
  }

  /// @brief Access a const element at the specified position.
  /// @param pos The position as a Vec2Si32.
  /// @return A const reference to the element at the specified position.
  const T& At(const Vec2Si32 pos) const {
    return data_[pos.x + pos.y * size_.x];
  }

  /// @brief Access an element at the specified coordinates.
  /// @param x The x-coordinate.
  /// @param y The y-coordinate.
  /// @return A reference to the element at the specified coordinates.
  T& At(const Si32 x, const Si32 y) {
    return data_[x + y * size_.x];
  }

  /// @brief Access a const element at the specified coordinates.
  /// @param x The x-coordinate.
  /// @param y The y-coordinate.
  /// @return A const reference to the element at the specified coordinates.
  const T& At(const Si32 x, const Si32 y) const {
    return data_[x + y * size_.x];
  }

  /// @brief Get the size of the Array2.
  /// @return The size of the Array2 as a Vec2Si32.
  Vec2Si32 Size() const {
    return size_;
  }

  /// @brief Check if the given position is within the bounds of the Array2.
  /// @param pos The position as a Vec2Si32 to check.
  /// @return True if pos is inside the bounds of the array, false otherwise.
  bool IsInBounds(const Vec2Si32 pos) const {
    return pos.x >= 0 && pos.y >= 0 && pos.x < size_.x && pos.y < size_.y;
  }

  /// @brief Check if the given coordinates are within the bounds of the Array2.
  /// @param x The x-coordinate to check.
  /// @param y The y-coordinate to check.
  /// @return True if x and y are inside the bounds of the array, false otherwise.
  bool IsInBounds(const Si32 x, const Si32 y) const {
    return x >= 0 && y >= 0 && x < size_.x && y < size_.y;
  }
};

}  // namespace arctic

#endif  // ENGINE_ARRAY2_H_
