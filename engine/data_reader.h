#pragma once

#include <vector>
#include "engine/arctic_types.h"

namespace arctic {

struct DataReader {
  std::vector<Ui8> data;
  Ui8 *p;
  Ui8 *end;

  void Reset(std::vector<Ui8> &&in_data);

  Ui64 Read(void *dst, Ui64 amount);
  Ui8 ReadUInt8();
  Ui16 ReadUInt16();
  Ui32 ReadUInt32();
  Ui64 ReadUInt64();
  float ReadFloat();
  void ReadFloatarray(float *dst, int num, int size, Ui64 amount);
  void ReadFloatarray2(float *dst, Ui64 amount);
  void ReadDoublearray2(double *dst, Ui64 amount);
  void ReadUInt64array(Ui64 *dst, Ui64 amount);
  void ReadUInt32array(Ui32 *dst, Ui64 amount);
  void ReadUInt32array2(Ui32 *dst, Ui64 amount);
  void ReadUInt16array(Ui16 *dst, Ui64 amount);
  void ReadUInt8array(Ui8 *dst, Ui64 amount);
};

} // namespace arctic
