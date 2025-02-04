// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "engine/arctic_platform_def.h"

#if defined (ARCTIC_PLATFORM_PI)

#include <cstring>
#include <iostream>

#ifdef ARCTIC_NO_HARD_EXIT
#include <setjmp.h>
extern jmp_buf arctic_jmp_env;
#endif  // ARCTIC_NO_HARD_EXIT

namespace arctic {


void Fatal(const char *message, const char *message_postfix) {
  size_t size = 1 +
    strlen(message) +
    (message_postfix ? strlen(message_postfix) : 0);
  char *full_message = static_cast<char *>(malloc(size));
  memset(full_message, 0, size);
  snprintf(full_message, size, "%s%s", message,
      (message_postfix ? message_postfix : ""));
#ifndef ARCTIC_NO_FATAL_MESSAGES
  std::cerr << "Arctic Engine ERROR: " << full_message << std::endl;
#endif  // ARCTIC_NO_FATAL_MESSAGES
#ifndef ARCTIC_NO_HARD_EXIT
  exit(1);
#else
  free(full_message);
  longjmp(arctic_jmp_env, 1337);
#endif  // ARCTIC_NO_HARD_EXIT
}

void Check(bool condition, const char *error_message,
    const char *error_message_postfix) {
  if (condition) {
    return;
  }
  Fatal(error_message, error_message_postfix);
}


}  // namespace arctic

#endif  // defined (ARCTIC_PLATFORM_PI)
