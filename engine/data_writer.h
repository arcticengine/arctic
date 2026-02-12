// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2025 Huldra
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

#pragma once

#include <vector>
#include "engine/arctic_types.h"

namespace arctic {

/// @brief A class for writing data to a buffer
struct DataWriter {
  std::vector<Ui8> data;

  /// @brief Write data to the buffer
  /// @param src The source of the data to write
  /// @param amount The amount of data to write (in bytes)
  /// @return The amount of data written (in bytes)
  Ui64 Write(const void *src, Ui64 amount);

  /// @brief Write an 8-bit unsigned integer to the buffer
  /// @param x The 8-bit unsigned integer to write
  void WriteUInt8(Ui8 x);

  /// @brief Write a 16-bit unsigned integer to the buffer
  /// @param x The 16-bit unsigned integer to write
  void WriteUInt16(Ui16 x);

  /// @brief Write a 32-bit unsigned integer to the buffer
  /// @param x The 32-bit unsigned integer to write
  void WriteUInt32(Ui32 x);

  /// @brief Write a 64-bit unsigned integer to the buffer
  /// @param x The 64-bit unsigned integer to write
  void WriteUInt64(Ui64 x);

  /// @brief Write a float to the buffer
  /// @param x The float to write
  void WriteFloat(float x);

  /// @brief Write an array of 8-bit unsigned integers to the buffer
  /// @param src The source array of 8-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt8array(Ui8 *src, Ui64 amount);

  /// @brief Write an array of 16-bit unsigned integers to the buffer
  /// @param src The source array of 16-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt16array(Ui16 *src, Ui64 amount);

  /// @brief Write an array of 32-bit unsigned integers to the buffer
  /// @param src The source array of 32-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt32array(Ui32 *src, Ui64 amount);

  /// @brief Write an array of 64-bit unsigned integers to the buffer
  /// @param src The source array of 64-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt64array(Ui64 *src, Ui64 amount);

  /// @brief Write an array of floats to the buffer
  /// @param src The source array of floats
  /// @param amount The number of elements in the array
  void WriteFloatarray2(float *src, Ui64 amount);

  /// @brief Write an array of doubles to the buffer
  /// @param src The source array of doubles
  /// @param amount The number of elements in the array
  void WriteDoublearray2(double *src, Ui64 amount);
};

} // namespace arctic
