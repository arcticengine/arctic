// The MIT License(MIT)
//
// Copyright 2015 - 2016 Inigo Quilez
// Copyright 2016 - 2017 Huldra
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

#include "engine/arctic_input.h"

#include <deque>
#include <mutex>  // NOLINT

#include "engine/arctic_platform.h"

namespace arctic {

static std::mutex g_input_mutex;
static std::deque<InputMessage> g_input;

bool PopInputMessage(InputMessage *out_message) {
  Check(out_message != nullptr, "Unexpected nullptr in out_message!");

  std::lock_guard<std::mutex> lock(g_input_mutex);

  auto it = g_input.begin();
  if (it == g_input.end()) {
    return false;
  }
  *out_message = *it;
  g_input.pop_front();
  return true;
}

void PushInputMessage(const InputMessage &message) {
  std::lock_guard<std::mutex> lock(g_input_mutex);
  g_input.push_back(message);
}

}  // namespace arctic
