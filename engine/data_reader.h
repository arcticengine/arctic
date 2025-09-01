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

/// @addtogroup global_utility
/// @{

/// @brief A class for reading data from a buffer
struct DataReader {
  std::vector<Ui8> data;
  Ui8 *p;
  Ui8 *end;

  /// @brief Reset the data reader with a new buffer
  /// @param in_data The new buffer to read from
  void Reset(std::vector<Ui8> &&in_data);

  /// @brief Read data from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in bytes)
  /// @return The amount of data read (in bytes)
  Ui64 Read(void *dst, Ui64 amount);

  /// @brief Read an 8-bit unsigned integer from the buffer
  /// @return The 8-bit unsigned integer read
  Ui8 ReadUInt8();

  /// @brief Read a 16-bit unsigned integer from the buffer
  /// @return The 16-bit unsigned integer read
  Ui16 ReadUInt16();

  /// @brief Read a 32-bit unsigned integer from the buffer
  /// @return The 32-bit unsigned integer read
  Ui32 ReadUInt32();
  
  /// @brief Read a 64-bit unsigned integer from the buffer
  /// @return The 64-bit unsigned integer read
  Ui64 ReadUInt64();

  /// @brief Read a float from the buffer
  float ReadFloat();
  
  /// @brief Read an array of floats from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in floats)
  void ReadFloatarray2(float *dst, Ui64 amount);

  /// @brief Read an array of doubles from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in doubles)
  void ReadDoublearray2(double *dst, Ui64 amount);

  /// @brief Read an array of 64-bit unsigned integers from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in 64-bit unsigned integers)
  void ReadUInt64array(Ui64 *dst, Ui64 amount);
  
  /// @brief Read an array of 32-bit unsigned integers from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in 32-bit unsigned integers)
  void ReadUInt32array(Ui32 *dst, Ui64 amount);

  /// @brief Read an array of 32-bit unsigned integers from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in 32-bit unsigned integers)
  void ReadUInt32array2(Ui32 *dst, Ui64 amount);
  
  /// @brief Read an array of 16-bit unsigned integers from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in 16-bit unsigned integers)
  void ReadUInt16array(Ui16 *dst, Ui64 amount);

  /// @brief Read an array of 8-bit unsigned integers from the buffer
  /// @param dst The destination to read to
  /// @param amount The amount of data to read (in 8-bit unsigned integers)
  void ReadUInt8array(Ui8 *dst, Ui64 amount);
  
};
/// @}



} // namespace arctic
