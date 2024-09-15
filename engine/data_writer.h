#pragma once

#include <vector>
#include "engine/arctic_types.h"

namespace arctic {

/// @brief A class for writing data to a buffer
struct DataWriter {
  std::vector<Ui8> data;

  /// @brief Write data to the buffer
  /// @param dst The source of the data to write
  /// @param amount The amount of data to write (in bytes)
  /// @return The amount of data written (in bytes)
  Ui64 Write(const void *dst, Ui64 amount);

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
  /// @param dst The source array of 8-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt8array(Ui8 *dst, Ui64 amount);

  /// @brief Write an array of 16-bit unsigned integers to the buffer
  /// @param dst The source array of 16-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt16array(Ui16 *dst, Ui64 amount);

  /// @brief Write an array of 32-bit unsigned integers to the buffer
  /// @param dst The source array of 32-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt32array(Ui32 *dst, Ui64 amount);

  /// @brief Write an array of 64-bit unsigned integers to the buffer
  /// @param dst The source array of 64-bit unsigned integers
  /// @param amount The number of elements in the array
  void WriteUInt64array(Ui64 *dst, Ui64 amount);

  /// @brief Write an array of floats to the buffer
  /// @param ori The source array of floats
  /// @param amount The number of elements in the array
  void WriteFloatarray2(float *ori, Ui64 amount);

  /// @brief Write an array of doubles to the buffer
  /// @param ori The source array of doubles
  /// @param amount The number of elements in the array
  void WriteDoublearray2(double *ori, Ui64 amount);
};

} // namespace arctic
