// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2017 - 2020 Huldra
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

#include "engine/unicode.h"

#include <cstring>

#include "engine/arctic_types.h"

namespace arctic {

void Utf32Reader::Reset(const Ui8 *data) {
  begin = data;
  p = data;
}

void Utf32Reader::Rewind() {
  p = begin;
}

Ui32 Utf32Reader::ReadOne() {
  while (true) {
    Ui32 u = 0;
    if ((p[0] & 0x80) == 0) {
      // 0xxxxxxx
      u = Ui32(p[0]);
      if (p[0] == 0) {
        return 0;
      }
      p++;
      return u;
    } else if ((p[0] & 0xe0) == 0xc0) {
      // 110xxxxx 10xxxxxx
      if ((p[1] & 0xc0u) == 0x80u) {
        u = (Ui32(p[0] & 0x1fu) << 6u) | (Ui32(p[1] & 0x3fu));
        p += 2;
        return u;
      }
    } else if ((p[0] & 0xf0) == 0xe0) {
      // 1110xxxx 10xxxxxx 10xxxxxx
      if ((p[1] & 0xc0u) == 0x80u && (p[2] & 0xc0u) == 0x80) {
        u = (Ui32(p[0] & 0x0fu) << 12u) | (Ui32(p[1] & 0x3fu) << 6u) |
          (Ui32(p[2] & 0x3fu));
        p += 3;
        return u;
      }
    } else if ((p[0] & 0xf8u) == 0xf0) {
      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      if ((p[1] & 0xc0u) == 0x80u && (p[2] & 0xc0u) == 0x80u &&
        (p[3] & 0xc0u) == 0x80u) {
        u = (Ui32(p[0] & 0x07u) << 18u) | (Ui32(p[1] & 0x3fu) << 12u) |
          (Ui32(p[2] & 0x3fu) << 6u) | (Ui32(p[3] & 0x3fu));
        p += 4;
        return u;
      }
    }
    p++;
  }
}


void Utf32FromUtf16::Reset(const Ui8 *data) {
  begin_ = data;
  p_ = data;
  is_inverse_byte_order_ = false;
}

void Utf32FromUtf16::Rewind() {
  p_ = begin_;
}

Ui32 Utf32FromUtf16::ReadOne() {
  while (true) {
    Ui16 s1 = Read16();
    while (s1 > 0xD7FFul && s1 < 0xE000ul) {
      if ((s1 & 0xFC00u) == 0xD800u) {
        Ui16 s2 = Read16();
        if ((s2 & 0xFC00u) == 0xDC00u) {
          Ui32 u = (static_cast<Ui32>(s1 & 0x3FFu) << 10u)
            + static_cast<Ui32>(s2 & 0x3FFu) + 0x10000ul;
          return u;
        }
        s1 = s2;
      } else {
        s1 = Read16();
      }
    }
    return s1;
  }
}

Ui16 Utf32FromUtf16::Read16() {
  Ui16 ch;
  if (is_inverse_byte_order_) {
    ch = static_cast<Ui16>(
        static_cast<Ui16>(p_[1]) | (static_cast<Ui16>(p_[0]) << 8u));
  } else {
    ch = static_cast<Ui16>(
        static_cast<Ui16>(p_[0]) | (static_cast<Ui16>(p_[1]) << 8u));
  }
  if (ch) {
    p_ += 2;
    if (ch == 0xFFFEu) {
      is_inverse_byte_order_ = !is_inverse_byte_order_;
      return 0xFEFFu;
    }
  }
  return ch;
}


void Utf8Codepoint::WriteUtf32(Ui32 codepoint) {
  if (codepoint <= 0x007Fu) {
    // 0xxxxxxx
    buffer[0] = static_cast<Ui8>(codepoint);
    size = 1;
    return;
  }
  if (codepoint <= 0x07FFul) {
    // 110xxxxx 10xxxxxx
    buffer[0] = static_cast<Ui8>((codepoint >> 6u) | 0xC0ul);
    buffer[1] = static_cast<Ui8>((codepoint & 0x3Ful) | 0x80ul);
    size = 2;
    return;
  }
  if (codepoint <= 0xFFFFul) {
    // 1110xxxx 10xxxxxx 10xxxxxx
    buffer[0] = static_cast<Ui8>((codepoint >> 12u) | 0xE0ul);
    buffer[1] = static_cast<Ui8>(((codepoint >> 6u) & 0x3Ful) | 0x80ul);
    buffer[2] = static_cast<Ui8>((codepoint & 0x3Ful) | 0x80ul);
    size = 3;
    return;
  }
  if (codepoint <= 0x10FFFFul) {
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    buffer[0] = static_cast<Ui8>((codepoint >> 18u) | 0xF0ul);
    buffer[1] = static_cast<Ui8>(((codepoint >> 12u) & 0x3Ful) | 0x80ul);
    buffer[2] = static_cast<Ui8>(((codepoint >> 6u) & 0x3Ful) | 0x80ul);
    buffer[3] = static_cast<Ui8>((codepoint & 0x3Ful) | 0x80ul);
    size = 4;
    return;
  }
  buffer[0] = 0;
  size = 0;
}

std::string Utf32ToUtf8(const void* data) {
  Ui64 size = 0;
  Utf8Codepoint cp;
  const Ui8* data_cursor = reinterpret_cast<const Ui8*>(data);
  while (true) {
    Ui32 d;
    memcpy(&d, data_cursor, 4);
    cp.WriteUtf32(d);
    if (d == 0) {
      break;
    }
    size += cp.size;
    data_cursor += sizeof(Ui32);
  }
  std::string buf;
  buf.resize(static_cast<size_t>(size));
  Ui32 pos = 0;

  data_cursor = reinterpret_cast<const Ui8*>(data);
  while (true) {
    Ui32 d;
    memcpy(&d, data_cursor, 4);
    cp.WriteUtf32(d);

    if (d == 0) {
      break;
    }
    Ui32 readPos = 0;
    while (readPos < cp.size) {
      buf[pos] = static_cast<char>(cp.buffer[readPos]);
      pos++;
      readPos++;
    }


    data_cursor += sizeof(Ui32);
  }
  return buf;
}

std::string Utf16ToUtf8(const void* data) {
  Ui64 size = 0;
  Utf8Codepoint cp;
  Utf32FromUtf16 converter;
  converter.Reset(reinterpret_cast<const Ui8*>(data));
  while (true) {
    Ui32 d = converter.ReadOne();
    if (d == 0) {
      break;
    }
    cp.WriteUtf32(d);
    size += cp.size;
  }
  std::string buf;
  buf.resize(static_cast<size_t>(size));
  Ui32 pos = 0;

  converter.Reset(reinterpret_cast<const Ui8*>(data));
  while (true) {
    Ui32 d = converter.ReadOne();
    if (d == 0) {
      break;
    }
    cp.WriteUtf32(d);

    Ui32 readPos = 0;
    while (readPos < cp.size) {
      buf[pos] = static_cast<char>(cp.buffer[readPos]);
      pos++;
      readPos++;
    }
  }
  return buf;
}

bool IsUtf8Continuation(Ui8 byte) {
  return (byte & 0xC0) == 0x80;
}

Si32 Utf8PrevCharPos(const std::string &s, Si32 pos) {
  if (pos <= 0) {
    return 0;
  }
  pos--;
  while (pos > 0 && IsUtf8Continuation(static_cast<Ui8>(s[static_cast<size_t>(pos)]))) {
    pos--;
  }
  return pos;
}

Si32 Utf8NextCharPos(const std::string &s, Si32 pos) {
  Si32 len = static_cast<Si32>(s.length());
  if (pos >= len) {
    return len;
  }
  pos++;
  while (pos < len && IsUtf8Continuation(static_cast<Ui8>(s[static_cast<size_t>(pos)]))) {
    pos++;
  }
  return pos;
}

}  // namespace arctic
