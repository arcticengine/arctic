#include <cstring>
#include "engine/data_writer.h"

namespace arctic {

Ui64 DataWriter::Write(const void *dst, Ui64 amount) {
  if (data.capacity() < data.size() + amount) {
    Ui64 needed = std::max(amount, (Ui64)(data.size())) + data.size();
    data.reserve((size_t)needed);
  }
  Ui8 *p = &data.back();
  data.resize(data.size() + (size_t)amount);
  memcpy(p, dst, (size_t)amount);
  return amount;
}

void DataWriter::WriteUInt8(Ui8 x) {
  Write(&x, 1);
}

void DataWriter::WriteUInt16(Ui16 x) {
  Write(&x, 2);
}

void DataWriter::WriteUInt32(Ui32 n) {
  Write(&n, 4);
}

void DataWriter::WriteUInt64(Ui64 n) {
  Write(&n, 8);
}

void DataWriter::WriteFloat(float x) {
  Write(&x, 4);
}

void DataWriter::WriteFloatarray2(float *ori, Ui64 amount) {
  Write(ori, 4*(size_t)amount);
}

void DataWriter::WriteDoublearray2(double *ori, Ui64 amount) {
  Write(ori, (size_t)amount*8);
}

void DataWriter::WriteUInt32array(Ui32 *dst, Ui64 amount) {
  Write(dst, amount*4);
}

void DataWriter::WriteUInt64array(Ui64 *dst, Ui64 amount) {
  Write(dst, amount*8);
}

void DataWriter::WriteUInt16array(Ui16 *dst, Ui64 amount) {
  Write(dst, amount*4);
}

}
