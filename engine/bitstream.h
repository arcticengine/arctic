// The MIT License (MIT)
//
// Copyright (c) 2020 The Lasting Curator
// Copyright (c) 2020 Huldra
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

#ifndef ENGINE_BITSTREAM_H_
#define ENGINE_BITSTREAM_H_

#include <deque>
#include <vector>

#include "engine/arctic_types.h"

namespace arctic {

class BitStream {
  // Single bit is represented with 1 byte of 10000000 or 0x80
  // sequence of 10001001 becomes 1 byte of 0x89
  // adding bits does not change preexisting ones
 protected:
  std::deque<Ui8> data_;
  Ui8 *write_cursor_;
  Ui8 unused_bits_ = 0;
  Ui8 *read_cursor_;
  Ui8 zero_ = 0;
  Ui64 read_byte_idx_ = 0;
  Ui64 read_bit_shift_ = 0;
 public:
  BitStream();
  explicit BitStream(std::vector<Ui8> &data);
  void PushBit(Ui64 bit);
  void BeginRead();
  Ui8 ReadBit();
  const std::deque<Ui8>& GetData();
};

}  // namespace arctic

#endif  // ENGINE_BITSTREAM_H_
