#pragma once

#include <vector>
#include "engine/arctic_types.h"

namespace arctic {

struct DataWriter {
  std::vector<Ui8> data;

  Ui64 Write(const void *dst, Ui64 amount);
  void WriteUInt8(Ui8 x);
  void WriteUInt16(Ui16 x);
  void WriteUInt32(Ui32 x);
  void WriteUInt64(Ui64 x);
  void WriteFloat(float x);
  void WriteUInt8array(Ui8 *dst, Ui64 amount);
  void WriteUInt16array(Ui16 *dst, Ui64 amount);
  void WriteUInt32array(Ui32 *dst, Ui64 amount);
  void WriteUInt64array(Ui64 *dst, Ui64 amount);
  void WriteFloatarray2(float *ori, Ui64 amout);
  void WriteDoublearray2(double *ori, Ui64 amout);
};

} // namespace piLibs
