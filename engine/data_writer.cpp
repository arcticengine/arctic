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
