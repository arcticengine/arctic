#include "engine/data_reader.h"

namespace arctic {

void DataReader::Reset(std::vector<Ui8> &&in_data) {
  data = std::move(in_data);
  if (data.empty()) {
    p = nullptr;
    end = nullptr;
  } else {
    p = &data[0];
    end = p + data.size();
  }
}

Ui64 DataReader::Read(void *dst, Ui64 amount) {
  Ui64 to_read = std::min(amount, (Ui64)(end - p));
  memcpy(dst, p, to_read);
  return to_read;
}

Ui8 DataReader::ReadUInt8() {
  Ui8 n;
  Read(&n, 1);
  return n;
}

Ui16 DataReader::ReadUInt16() {
  Ui16 n;
  Read(&n, 2);
  return n;
}

Ui32 DataReader::ReadUInt32() {
  unsigned int n;
  Read(&n, 4);
  return n;
}

Ui64 DataReader::ReadUInt64() {
  Ui64 n;
  Read(&n, 8);
  return n;
}

float DataReader::ReadFloat() {
  float n;
  Read(&n, 4);
  return n;
}

void DataReader::ReadFloatarray(float *dst, int num, int size, Ui64 amount) {
  Ui8 *ptr = (Ui8 *)dst;
  for (Ui64 i=0; i<amount; i++) {
    Read(ptr, num*4);
    ptr += size;
  }
}

void DataReader::ReadFloatarray2(float *dst, Ui64 amount) {
  Read(dst, amount*4);
}

void DataReader::ReadUInt32array(Ui32 *dst, Ui64 amount) {
  Read(dst, amount*4);
}


void DataReader::ReadUInt64array(Ui64 *dst, Ui64 amount) {
  Read(dst, amount*8);
}

void DataReader::ReadUInt32array2(Ui32 *dst, Ui64 amount) {
  Read(dst, amount*4);
}

void DataReader::ReadUInt16array(Ui16 *dst, Ui64 amount) {
  Read(dst, amount*2);
}

void DataReader::ReadUInt8array(Ui8 *dst, Ui64 amount) {
  Read(dst, amount);
}

void DataReader::ReadDoublearray2(double *dst, Ui64 amount) {
  Read(dst, amount*8);
}

}
