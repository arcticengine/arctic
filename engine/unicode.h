// The MIT License (MIT)
//
// Copyright (c) 2017 - 2019 Huldra
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

#ifndef ENGINE_UNICODE_H_
#define ENGINE_UNICODE_H_

#include <string>

#include "engine/arctic_types.h"

namespace arctic {

/// @addtogroup global_utility
/// @{

struct Utf32Reader {
  const Ui8 *begin = nullptr;
  const Ui8 *p = nullptr;

  void Reset(const Ui8 *data);
  void Rewind();
  Ui32 ReadOne();  // Read one Utf32 character while converting it from Utf8
};

class Utf32FromUtf16 {
 public:
  void Reset(const Ui8 *data);
  void Rewind();
  Ui32 ReadOne();

 protected:
  const Ui8 *begin_ = nullptr;
  const Ui8 *p_ = nullptr;
  bool is_inverse_byte_order_ = false;
  Ui16 Read16();
};

struct Utf8Codepoint {
  Ui64 size = 0;
  Ui8 buffer[4] = {0, 0, 0, 0};
  void WriteUtf32(Ui32 codepoint);
};


/// @brief Convers a UTF-32 encoded string to UTF-8.
/// @param [in] data Address of the UTF-32 encoded string.
/// @result UTF-8 std::string.
std::string Utf32ToUtf8(const void* data);

/// @brief Convers a UTF-16 encoded string to UTF-8.
/// @param [in] data Address of the UTF-16 encoded string.
/// @result UTF-8 std::string.
std::string Utf16ToUtf8(const void* data);

/// @}

}  // namespace arctic

#endif  // ENGINE_UNICODE_H_
