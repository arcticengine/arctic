// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 - 2017 Huldra
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

#include <cstddef>

#include "engine/arctic_input.h"
#include "engine/arctic_platform.h"
#include "engine/mtq_mpsc_vinfarr.h"
#include "engine/mtq_spmc_array.h"

namespace arctic {

static MpmcBestEffortFixedSizeBufferFixedSizePool<8, 4080> g_input_page_pool;
static MpscVirtInfArray<InputMessage*, TuneDeletePayloadFlag<true>, TuneMemoryPoolFlag<true>> g_input(
    &g_input_page_pool);
static SpmcArray<InputMessage, true> g_input_pool(1000);

bool PopInputMessage(InputMessage *out_message) {
  Check(out_message != nullptr, "Unexpected nullptr in out_message!");

  InputMessage *msg = g_input.dequeue();
  if (msg == nullptr) {
    return false;
  }
  *out_message = *msg;
  if (!g_input_pool.enqueue(msg)) {
    delete msg;
  }
  return true;
}

void PushInputMessage(const InputMessage &message) {
  InputMessage *p = g_input_pool.dequeue();
  if (p == nullptr) {
    g_input.enqueue(new InputMessage(message));
  } else {
    *p = message;
    g_input.enqueue(p);
  }
}

bool IsTypedTextByte(char byte) {
  const Ui8 value = static_cast<Ui8>(byte);
  return value >= 0x20 && value != 0x7f;
}

bool SetTypedCharacters(InputMessage::Keyboard *out_keyboard, const char *utf8) {
  Check(out_keyboard != nullptr,
    "Unexpected nullptr in SetTypedCharacters call");
  const size_t size = sizeof(out_keyboard->characters);
  size_t length = 0;
  if (utf8 != nullptr) {
    for (const char *p = utf8; *p != '\0' && length + 1 < size; ++p) {
      if (IsTypedTextByte(*p)) {
        out_keyboard->characters[length] = *p;
        ++length;
      }
    }
  }
  for (size_t i = length; i < size; ++i) {
    out_keyboard->characters[i] = '\0';
  }
  return length != 0;
}

}  // namespace arctic
