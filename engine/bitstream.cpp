// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/bitstream.h"

namespace arctic {

BitStream::BitStream()
  : write_cursor_(&zero_)
  , read_cursor_(&zero_) {}

BitStream::BitStream(std::vector<Ui8> &data)
    : write_cursor_(&zero_)
    , read_cursor_(&zero_) {
  data_.resize(data.size());
  std::copy(data.begin(), data.end(), data_.begin());
  BeginRead();
}

void BitStream::PushBit(Ui64 bit) {
  if (unused_bits_) {
    --unused_bits_;
    *write_cursor_ |= ((bit & 1ull) << unused_bits_);
  } else {
    data_.push_back(static_cast<Ui8>((bit & 1ull) << 7u));
    write_cursor_ = &data_.back();
    unused_bits_ = 7;
  }
}

void BitStream::BeginRead() {
  read_byte_idx_ = 0;
  read_bit_shift_ = 7;
  if (!data_.empty()) {
    read_cursor_ = &data_.front();
  } else {
    read_cursor_ = &zero_;
  }
}

Ui8 BitStream::ReadBit() {
  Ui8 bit = (((*read_cursor_) >> read_bit_shift_) & 1ull);
  if (read_bit_shift_ > 0) {
    --read_bit_shift_;
  } else {
    read_bit_shift_ = 7;
    read_byte_idx_++;
    if (read_byte_idx_ < data_.size()) {
      read_cursor_ = &data_[read_byte_idx_];
    } else {
      read_cursor_ = &zero_;
    }
  }
  return bit;
}

const std::deque<Ui8>& BitStream::GetData() {
  return data_;
}

}  // namespace arctic

