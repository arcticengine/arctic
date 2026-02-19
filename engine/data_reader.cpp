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

#include <cstring>
#include "engine/data_reader.h"

namespace arctic {

void DataReader::Reset(std::vector<Ui8> &&in_data) {
  data = std::move(in_data);
  is_ok_ = true;
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
  memcpy(dst, p, (size_t)to_read);
  if (to_read < amount) {
    memset(static_cast<Ui8*>(dst) + to_read, 0, (size_t)(amount - to_read));
    is_ok_ = false;
  }
  p += to_read;
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
  Ui32 n;
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
