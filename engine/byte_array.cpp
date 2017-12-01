// The MIT License(MIT)
//
// Copyright 2017 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
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

#include "engine/byte_array.h"

#include <stdlib.h>
#include <cstring>

#include "engine/arctic_platform.h"

namespace arctic {

ByteArray::ByteArray() {
  allocated_size_ = 128;
  size_ = 0;
  data_ = static_cast<Ui8*>(malloc(static_cast<size_t>(allocated_size_)));
}

ByteArray::ByteArray(Ui64 size) {
  allocated_size_ = size;
  size_ = 0;
  data_ = static_cast<Ui8*>(malloc(static_cast<size_t>(allocated_size_)));
}

ByteArray::~ByteArray() {
  allocated_size_ = 0;
  size_ = 0;
  free(data_);
  data_ = nullptr;
}

void* ByteArray::GetVoidData() const {
  return static_cast<void*>(data_);
}

Ui8* ByteArray::data() const {
  return data_;
}

Ui64 ByteArray::size() const {
  return size_;
}

void ByteArray::Resize(Ui64 size) {
  if (size <= allocated_size_) {
    size_ = size;
  } else {
    Ui8 *data = static_cast<Ui8*>(malloc(static_cast<size_t>(size)));
    Check(data != nullptr, "Allocaton error.");
    std::memcpy(data, data_, static_cast<size_t>(size_));
    free(data_);
    allocated_size_ = size;
    size_ = size;
    data_ = data;
  }
}

void ByteArray::Reserve(Ui64 size) {
  if (size > size_) {
    Ui64 oldSize = size_;
    Resize(size);
    Resize(oldSize);
  }
}

}  // namespace arctic
